#pragma once

#include <vector>
#include <memory>
#include <stdexcept>
#include <exception>
#include <functional>
#include "Scripting/Tokens.h"
#include "Scripting/Parser/ast.h"
#include "Scripting/Interpreter/Environment.h"

namespace ObSL {
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

    struct RuntimeError : public std::runtime_error {
        Token token;

        RuntimeError(const Token &token, const std::string &msg)
            : std::runtime_error(msg), token(token) {
        }
    };

    class Interpreter {
    private:
        std::shared_ptr<Environment> globals;
        std::shared_ptr<Environment> environment;
        std::vector<std::vector<std::unique_ptr<Stmt> > > ast_storage;

    public:
        Interpreter() {
            globals = std::make_shared<Environment>();
            environment = globals;
        }

        void interpret(std::vector<std::unique_ptr<Stmt> > statements);

        void execute_block(const std::vector<std::unique_ptr<Stmt> > &statements,
                           std::shared_ptr<Environment> block_env);

        void define_native(const std::string &name,
                           std::shared_ptr<ObSLCallable> function) const {
            globals->define(name, function);
        }

    private:
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

        void execute_while_stmt(const WhileStmt *stmt);

        void execute_break_stmt(const BreakStmt *stmt);

        void execute_return_stmt(const ReturnStmt *stmt);

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
    };

    class ObSLFunction : public ObSLCallable {
    private:
        const FunctionStmt *declaration;
        std::shared_ptr<Environment> closure;

    public:
        ObSLFunction(const FunctionStmt *declaration, std::shared_ptr<Environment> closure);

        [[nodiscard]] int arity() const override;

        Value call(Interpreter *interpreter, const std::vector<Value> &arguments) override;

        [[nodiscard]] std::string to_string() const override;
    };

    // for binding C++ lambdas / functions to ObSL callables
    class NativeFunction : public ObSLCallable {
    private:
        int m_arity;
        std::function<Value(Interpreter *, const std::vector<Value> &)> m_body;
        std::string m_name;

    public:
        NativeFunction(const int arity,
                       std::function<Value
                           (Interpreter *, const std::vector<Value> &)> body,
                       std::string name = "native")
            : m_arity(arity), m_body(std::move(body)), m_name(std::move(name)) {
        }

        [[nodiscard]] int arity() const override {
            return m_arity;
        }

        Value call(Interpreter *interpreter, const std::vector<Value> &arguments) override {
            return m_body(interpreter, arguments);
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("<native fn{}>", m_name);
        }
    };
} // namespace ObSL
