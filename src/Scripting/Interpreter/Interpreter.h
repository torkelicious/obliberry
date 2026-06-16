#pragma once

#include <vector>
#include <memory>
#include <stdexcept>
#include <exception>
#include <functional>
#include <tuple>
#include <utility>
#include <type_traits>
#include <unordered_set>
#include <string_view>
#include <format>
#include <iostream>
#include <algorithm>
#include "Scripting/Tokens.h"
#include "Scripting/Parser/ast.h"
#include "Scripting/Interpreter/Environment.h"
#include "Scripting/StdLib/StdLib.h"

namespace ObSL {
    // forward declarations
    class NativeFunction;
    struct ObSLCallable;
    class Interpreter;

    template<typename T>
    struct native_fn_traits;

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
        else if constexpr (std::is_same_v<T, std::shared_ptr<ObSLCallable> >) return "callable";
        else if constexpr (std::is_same_v<T, std::shared_ptr<ObSLArray> >) return "array";
        else if constexpr (std::is_same_v<T, std::shared_ptr<ObSLObject> >) return "object";
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
    };

    struct NativeTypeError : public std::runtime_error {
        explicit NativeTypeError(const std::string &msg)
            : std::runtime_error(msg) {
        }
    };

    class NativeObjectCreator : public ObSLCallable {
    public:
        [[nodiscard]] int arity() const override { return 0; }

        Value call(Interpreter *interpreter, const std::vector<Value> &arguments, const Token &call_token) override {
            return std::make_shared<ObSLObject>();
        }

        [[nodiscard]] std::string to_string() const override { return "<native fn Object>"; }
    };

    // Interpreter
    class Interpreter {
    public:
        explicit Interpreter(std::ostream &out = std::cout, std::istream &in = std::cin)
            : m_stdout(out), m_stdin(in) {
            globals = std::make_shared<Environment>();
            register_environment(globals);
            environment = globals;
            globals->define("Object", std::make_shared<NativeObjectCreator>());

            StdLib std_lib;
            std_lib.register_modules(*this);
        }

        ~Interpreter() {
            if (globals) {
                globals->clear();
            }

            // stop leftover reference cycles trapped in closure scopes
            for (auto &weak_env: all_environments) {
                if (auto env = weak_env.lock()) {
                    env->clear();
                }
            }
        }

        void register_environment(const std::shared_ptr<Environment> &env) {
            // periodically prune expired pointers just incase
            if (all_environments.size() > 1000) {
                all_environments.erase(
                    std::ranges::remove_if(all_environments,
                                           [](const std::weak_ptr<Environment> &wp) { return wp.expired(); }).begin(),
                    all_environments.end());
            }
            all_environments.push_back(env);
        }

        void interpret(std::vector<std::unique_ptr<Stmt> > statements);

        void execute_block(const std::vector<std::unique_ptr<Stmt> > &statements,
                           std::shared_ptr<Environment> block_env);

        void define_native(const std::string &name, std::shared_ptr<ObSLCallable> function) const {
            globals->define(name, function);
        }

        // registration wrapper
        template<typename F>
        void define_native(std::string name, F &&body);

        std::istream &Get_Stdin() const { return m_stdin; }
        std::ostream &Get_Stdout() const { return m_stdout; }

        [[nodiscard]] std::shared_ptr<Environment> get_current_environment() const {
            return environment;
        }

        void set_current_environment(std::shared_ptr<Environment> env) {
            environment = std::move(env);
        }

        friend class ObSLFunction;

    private:
        std::shared_ptr<Environment> globals;
        std::shared_ptr<Environment> environment;
        std::vector<std::vector<std::unique_ptr<Stmt> > > ast_storage;
        std::vector<std::string> source_storage;
        std::vector<std::weak_ptr<Environment> > all_environments;
        std::ostream &m_stdout;
        std::istream &m_stdin;
        std::unordered_map<std::string, std::shared_ptr<ObSLObject> > loaded_modules;

        void execute(const Stmt *stmt);

        Value evaluate(const Expr *expr);

        void execute_function_stmt(const FunctionStmt *stmt);

        Value evaluate_call(const CallExpr *expr);

        void execute_expression_stmt(const ExpressionStmt *stmt);

        void execute_print_stmt(const PrintStmt *stmt);

        void execute_print_ln_stmt(const PrintlnStmt *stmt);

        void execute_var_stmt(const VarStmt *stmt);

        void execute_block_stmt(const BlockStmt *stmt);

        void execute_if_stmt(const IfStmt *stmt);

        void execute_switch_stmt(const SwitchStmt *stmt);

        void execute_while_stmt(const WhileStmt *stmt);

        void execute_foreach_stmt(const ForeachStmt *stmt);

        void execute_break_stmt(const BreakStmt *stmt);

        void execute_return_stmt(const ReturnStmt *stmt);

        Value evaluate_get(const GetExpr *expr);

        Value evaluate_set(const SetExpr *expr);

        Value evaluate_literal(const LiteralExpr *expr);

        Value evaluate_variable(const VariableExpr *expr) const;

        Value evaluate_array(const ArrayExpr *expr);

        Value evaluate_index(const IndexExpr *expr);

        Value evaluate_index_assignment(const IndexAssignmentExpr *expr);

        Value evaluate_binary(const BinaryExpr *expr);

        Value evaluate_grouping(const GroupingExpr *expr);

        Value evaluate_unary(const UnaryExpr *expr);

        Value evaluate_update(const UpdateExpr *expr);

        Value evaluate_assignment(const AssignmentExpr *expr);

        Value evaluate_logical(const LogicalExpr *expr);

        bool is_truthy(const Value &value);

        void check_number_operand(const Token &oprt, const Value &oprnd);

        void check_number_operands(const Token &oprt, const Value &lhs, const Value &rhs);

        [[nodiscard]] bool is_equal(const Value &a, const Value &b) const;

        void execute_using_stmt(const UsingStmt *stmt);

        void execute_try_catch_stmt(const TryCatchStmt *stmt);

        template<typename TargetType>
        static void validate_native_arg(const Value &arg, const size_t index) {
            using DecayedTarget = std::decay_t<TargetType>;
            if constexpr (std::is_same_v<DecayedTarget, Value>) {
                return; // A raw Value is always valid, skip variant alternative checking!
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

            // Helper to safely extract the argument depending on if it's already a variant
            auto unpack = []<typename T>(const Value &v) -> decltype(auto) {
                using DecayedT = std::decay_t<T>;
                if constexpr (std::is_same_v<DecayedT, Value>) {
                    return v;
                } else {
                    return std::get<DecayedT>(v);
                }
            };

            if constexpr (std::is_void_v<typename Traits::return_type>) {
                body(unpack.template operator()<std::tuple_element_t<Is, ArgsTuple> >(args[Is])...);
                return std::monostate{};
            } else {
                return body(unpack.template operator()<std::tuple_element_t<Is, ArgsTuple> >(args[Is])...);
            }
        }
    };

    // user defined functions
    class ObSLFunction : public ObSLCallable {
    private:
        const FunctionStmt *declaration;
        std::shared_ptr<Environment> closure;

    public:
        ObSLFunction(const FunctionStmt *declaration, std::shared_ptr<Environment> closure);

        [[nodiscard]] int arity() const override;

        [[nodiscard]] int min_arity() const override;

        Value call(Interpreter *interpreter, const std::vector<Value> &arguments, const Token &call_token) override;

        [[nodiscard]] std::string to_string() const override;

        std::shared_ptr<ObSLFunction> bind(std::shared_ptr<ObSLObject> instance, Interpreter *interpreter = nullptr) {
            auto enviroment = std::make_shared<Environment>(closure);
            if (interpreter) {
                interpreter->register_environment(enviroment);
            }
            enviroment->define("this", instance);
            return std::make_shared<ObSLFunction>(declaration, enviroment);
        }
    };

    // native binds
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
            } catch (const RuntimeError &e) {
                throw;
            } catch (const std::exception &e) {
                throw RuntimeError(call_token, std::format("Native function '{}': {}", m_name, e.what()));
            }
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("<native fn {}>", m_name);
        }
    };

    template<typename F>
    void Interpreter::define_native(std::string name, F &&body) {
        using DecayedF = std::decay_t<F>;
        if constexpr (std::is_pointer_v<DecayedF> && std::is_function_v<std::remove_pointer_t<DecayedF> >) {
            using Traits = native_fn_traits<DecayedF>;
            auto wrapped = [body = std::forward<F>(body)](Interpreter *, const std::vector<Value> &args) -> Value {
                return call_native_helper<DecayedF, Traits>(body, args, std::make_index_sequence<Traits::arity>{});
            };
            globals->define(name, std::make_shared<NativeFunction>(Traits::arity, std::move(wrapped), name));
        } else {
            using Traits = native_fn_traits<decltype(&DecayedF::operator())>;
            auto wrapped = [body = std::forward<F>(body)](Interpreter *, const std::vector<Value> &args) -> Value {
                return call_native_helper<DecayedF, Traits>(body, args, std::make_index_sequence<Traits::arity>{});
            };
            globals->define(name, std::make_shared<NativeFunction>(Traits::arity, std::move(wrapped), name));
        }
    }
} // namespace ObSL
