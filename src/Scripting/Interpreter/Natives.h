#pragma once

#include <vector>
#include <memory>
#include <stdexcept>
#include <exception>
#include <functional>
#include <tuple>
#include <utility>
#include <type_traits>
#include <string_view>
#include <format>
#include <variant>

#include "Scripting/Tokens.h"
#include "Scripting/Parser/ast.h"

namespace ObSL {
    class Interpreter;

    template<typename T>
    struct native_fn_traits;

    // concepts for function traits
    template<typename F>
    concept FunctionPointer = std::is_function_v<std::remove_pointer_t<F> >;

    template<typename F>
    concept MemberFunctionPointer = std::is_member_function_pointer_v<F>;

    // for const member functions
    template<typename R, typename C, typename... Args>
    struct native_fn_traits<R(C::*)(Args...) const> {
        using return_type = R;
        static constexpr int arity = sizeof...(Args);
        using args_tuple = std::tuple<std::decay_t<Args>...>;
    };

    // for non-const member functions
    template<typename R, typename C, typename... Args>
    struct native_fn_traits<R(C::*)(Args...)> {
        using return_type = R;
        static constexpr int arity = sizeof...(Args);
        using args_tuple = std::tuple<std::decay_t<Args>...>;
    };

    // free function pointers
    template<typename R, typename... Args>
    struct native_fn_traits<R(*)(Args...)> {
        using return_type = R;
        static constexpr int arity = sizeof...(Args);
        using args_tuple = std::tuple<std::decay_t<Args>...>;
    };

    // runtime Exceptions
    struct BreakException : public std::exception {
        [[nodiscard]] const char *what() const noexcept override {
            return "Break signal";
        }
    };

    struct ReturnException : public std::exception {
        Value value;

        explicit ReturnException(Value value) : value(std::move(value)) {
        }

        [[nodiscard]] const char *what() const noexcept override {
            return "Return signal";
        }
    };

    template<typename T>
    consteval std::string_view native_type_name() {
        if constexpr (std::is_same_v<T, bool>) return "bool";
        else if constexpr (std::is_same_v<T, double>) return "number";
        else if constexpr (std::is_same_v<T, std::string>) return "string";
        else if constexpr (std::is_same_v<T, ObSLCallable *>) return "callable";
        else if constexpr (std::is_same_v<T, ObSLArray *>) return "array";
        else if constexpr (std::is_same_v<T, ObSLObject *>) return "object";
        else if constexpr (std::is_same_v<T, std::monostate>) return "null";
        else return "unknown";
    }

    struct RuntimeError : public std::runtime_error {
        Token token;

        RuntimeError(const Token &token, std::string_view message)
            : std::runtime_error(std::format("[Line {}:{}] Error at '{}': {}",
                                             token.line, token.column, token.lexeme, message)),
              token(token) {
        }

        RuntimeError(const std::string_view name, std::string_view message)
            : std::runtime_error(std::format("Error: {}", message)),
              token(Token{TokenType::UNKNOWN, name, 0, 0, 0, 0}) {
        }
    };

    struct NativeTypeError : public std::runtime_error {
        explicit NativeTypeError(const std::string &msg)
            : std::runtime_error(msg) {
        }
    };

    class NativeFunction : public ObSLCallable {
    private:
        int m_arity;
        std::function<Value(Interpreter *, const std::vector<Value> &)> m_body;
        std::string m_name;

    public:
        NativeFunction(const int arity,
                       std::function<Value(Interpreter *, const std::vector<Value> &)> body,
                       std::string name = "native")
            : m_arity(arity), m_body(std::move(body)), m_name(std::move(name)) {
        }

        [[nodiscard]] int arity() const override { return m_arity; }

        Value call(Interpreter *interpreter, const std::vector<Value> &arguments, const Token &call_token) override {
            if (arguments.size() != m_arity) {
                throw RuntimeError(call_token,
                                   std::format("Native function expects {} arguments but got {}.", m_arity,
                                               arguments.size()));
            }
            try {
                return m_body(interpreter, arguments);
            } catch (RuntimeError) {
                throw;
            } catch (const std::exception &e) {
                throw RuntimeError(call_token, std::format("Native function '{}': {}", m_name, e.what()));
            }
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("<native fn {}>", m_name);
        }

        void mark() override {
            is_marked = true;
        }
    };

    template<typename TargetType>
    static void validate_native_arg(const Value &arg, const size_t index) {
        using DecayedTarget = std::decay_t<TargetType>;
        if constexpr (std::is_same_v<DecayedTarget, Value>) {
        } else {
            if (!std::holds_alternative<DecayedTarget>(arg)) {
                throw NativeTypeError(
                    std::format("Argument {}: expected type '{}', got '{}'.",
                                index,
                                native_type_name<DecayedTarget>(),
                                std::visit([]<typename T>(const T &) -> std::string_view {
                                    return native_type_name<std::decay_t<T> >();
                                }, arg))
                );
            }
        }
    }

    template<typename F, typename Traits, size_t... Is>
    static Value call_native_helper(const F &body, const std::vector<Value> &args, std::index_sequence<Is...>) {
        using ArgsTuple = Traits::args_tuple;
        (
            validate_native_arg<std::tuple_element_t<Is, ArgsTuple> >(args[Is], Is),
            ...
        );

        auto unpack = []<typename T>(const Value &v) -> decltype(auto) {
            using DecayedT = std::decay_t<T>;
            if constexpr (std::is_same_v<DecayedT, Value>) {
                return v;
            } else {
                return std::get<DecayedT>(v);
            }
        };

        if constexpr (std::is_void_v < typename Traits::return_type >) {
            body(unpack.template operator()<std::tuple_element_t<Is, ArgsTuple> >(args[Is])...);
            return std::monostate{};
        } else {
            return body(unpack.template operator()<std::tuple_element_t<Is, ArgsTuple> >(args[Is])...);
        }
    }
} // namespace ObSL
