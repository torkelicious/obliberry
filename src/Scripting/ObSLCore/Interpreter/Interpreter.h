#pragma once

#include <iostream>
#include <istream>
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Scripting/ObSLCore/GarbageCollector.h"
#include "Scripting/ObSLCore/Interpreter/Environment.h"
#include "Scripting/ObSLCore/Interpreter/Natives.h"
#include "Scripting/ObSLCore/StdLib/StdLib.h"

namespace ObSL {
    class Interpreter {
    public:
        GarbageCollector gc{this};
        std::vector<Value> gc_protect_stack;

        void *user_data = nullptr;

        explicit Interpreter(std::ostream &out = std::cout, std::istream &in = std::cin) : m_stdout(out), m_stdin(in) {
            globals = std::make_shared<Environment>();
            register_environment(globals);
            environment = globals;

            define_native("Object", [this]() -> ObSLObject * { return this->gc.allocate<ObSLObject>(); });

            StdLib::register_modules(*this);
        }

        ~Interpreter() {
            std::unique_lock lock(m_interpreter_mutex);
            if (globals)
                globals->clear();
            for (auto &weak_env: all_environments) {
                if (const auto env = weak_env.lock())
                    env->clear();
            }
        }

        void register_environment(const std::shared_ptr<Environment> &env) {
            std::unique_lock lock(m_interpreter_mutex);
            if (++m_env_insert_count % prune_interval == 0) {
                std::erase_if(all_environments, [](const std::weak_ptr<Environment> &wp) { return wp.expired(); });
            }
            all_environments.push_back(env);
        }

        void interpret(const std::vector<std::unique_ptr<Stmt> > &statements);

        void execute_block(std::span<const std::unique_ptr<Stmt>> statements, std::shared_ptr<Environment> block_env);

        void define_native(const std::string &name, ObSLCallable *function) const { globals->define(name, function); }

        template<typename F>
        void define_native(std::string name, F &&body);

        void Set_Stdout(std::ostream &out) { m_stdout = std::ref(out); }
        void Set_Stdin(std::istream &in) { m_stdin = std::ref(in); }

        std::istream &Get_Stdin() const { return m_stdin.get(); }
        std::ostream &Get_Stdout() const { return m_stdout.get(); }

        [[nodiscard]] std::shared_ptr<Environment> get_current_environment() const {
            std::unique_lock lock(m_interpreter_mutex);
            return environment;
        }

        [[nodiscard]] std::shared_ptr<Environment> get_global_environment() const {
            std::unique_lock lock(m_interpreter_mutex);
            return globals;
        }

        void set_current_environment(std::shared_ptr<Environment> env) {
            std::unique_lock lock(m_interpreter_mutex);
            environment = std::move(env);
        }

        void mark_roots() {
            std::unique_lock lock(m_interpreter_mutex);
            if (globals)
                globals->mark();
            for (auto &weak_env: all_environments) {
                if (const auto env = weak_env.lock())
                    env->mark();
            }
            if (environment)
                environment->mark();
            for (auto &val: gc_protect_stack) {
                mark_value(val);
            }
            for (const auto &module_obj: loaded_modules | std::views::values) {
                if (module_obj)
                    module_obj->mark();
            }
        }

        friend class ObSLFunction;
        friend struct ObSLStruct;

    private:
        static constexpr std::size_t prune_interval = 64;
        static constexpr std::size_t max_loaded_modules = 256;

        mutable std::recursive_mutex m_interpreter_mutex;
        mutable std::shared_mutex m_modules_mutex;

        // Stream wrappers must be declared first to ensure they are fully initialized
        // before other members that might use them during construction (e.g., StdLib).
        std::reference_wrapper<std::ostream> m_stdout;
        std::reference_wrapper<std::istream> m_stdin;

        std::shared_ptr<Environment> globals;
        std::shared_ptr<Environment> environment;
        std::vector<std::weak_ptr<Environment> > all_environments;

        std::unordered_map<std::string, ObSLObject *> loaded_modules;

        std::vector<std::string> module_sources;
        std::vector<std::vector<std::unique_ptr<Stmt> > > module_asts;

        std::size_t m_env_insert_count = 0;

        void execute(const Stmt *stmt);

        [[nodiscard]] Value evaluate(const Expr *expr);

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

        static void execute_break_stmt(const BreakStmt *stmt);

        void execute_return_stmt(const ReturnStmt *stmt);

        Value evaluate_get(const GetExpr *expr);

        Value evaluate_set(const SetExpr *expr);

        static Value evaluate_literal(const LiteralExpr *expr);

        Value evaluate_variable(const VariableExpr *expr) const;

        Value evaluate_array(const ArrayExpr *expr);

        Value evaluate_index(const IndexExpr *expr);

        Value evaluate_index_assignment(const IndexAssignmentExpr *expr);

        Value evaluate_binary(const BinaryExpr *expr);

        Value evaluate_grouping(const GroupingExpr *expr);

        Value evaluate_unary(const UnaryExpr *expr);

        Value evaluate_update(const UpdateExpr *expr) const;

        Value evaluate_assignment(const AssignmentExpr *expr);

        Value evaluate_logical(const LogicalExpr *expr);

        Value evaluate_type_check(const TypeCheckExpr *expr);

        static bool is_truthy(const Value &value);

        static void check_number_operand(TokenType oprt, const Value &oprnd);

        static void check_number_operands(TokenType oprt, const Value &lhs, const Value &rhs);

        static bool is_equal(const Value &a, const Value &b);

        void execute_using_stmt(const UsingStmt *stmt);

        void execute_try_catch_stmt(const TryCatchStmt *stmt);

        void execute_struct_stmt(const StructStmt *stmt);
    };

    struct GCProtectScope {
        Interpreter *interpreter;
        size_t start_size;

        explicit GCProtectScope(Interpreter *interp) : interpreter(interp),
                                                       start_size(interp->gc_protect_stack.size()) {
        }

        ~GCProtectScope() { interpreter->gc_protect_stack.resize(start_size); }

        void protect(const Value &val) const { interpreter->gc_protect_stack.push_back(val); }
    };

    class ObSLFunction : public ObSLCallable {
    private:
        const FunctionStmt *declaration;
        std::shared_ptr<Environment> closure;

    public:
        ObSLFunction(const FunctionStmt *declaration, std::shared_ptr<Environment> closure)
            : declaration(declaration), closure(std::move(closure)) {
        }

        [[nodiscard]] int arity() const override;

        [[nodiscard]] int min_arity() const override;

        Value call(Interpreter *interpreter, const std::vector<Value> &arguments, const Token &call_token) override;

        [[nodiscard]] std::string to_string() const override;

        void mark() override {
            if (is_marked)
                return;
            is_marked = true;
            if (closure)
                closure->mark();
        }

        ObSLFunction *bind(ObSLObject *instance, Interpreter *interpreter) {
            auto enviroment = std::make_shared<Environment>(closure);
            interpreter->register_environment(enviroment);
            enviroment->define("this", instance);

            return interpreter->gc.allocate<ObSLFunction>(declaration, enviroment);
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
            globals->define(name, gc.allocate<NativeFunction>(Traits::arity, std::move(wrapped), name));
        } else {
            using Traits = native_fn_traits<decltype(&DecayedF::operator())>;
            auto wrapped = [body = std::forward<F>(body)](Interpreter *, const std::vector<Value> &args) -> Value {
                return call_native_helper<DecayedF, Traits>(body, args, std::make_index_sequence<Traits::arity>{});
            };
            globals->define(name, gc.allocate<NativeFunction>(Traits::arity, std::move(wrapped), name));
        }
    }
} // namespace ObSL
