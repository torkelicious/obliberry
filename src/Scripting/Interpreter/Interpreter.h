#ifndef OBLIBERRY_INTERPRETER_H
#define OBLIBERRY_INTERPRETER_H
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
        RuntimeError(const Token &token, const std::string &msg) : std::runtime_error(msg), token(token) {
        }
    };

    class Interpreter {
    private:
        // the global scope
        std::shared_ptr<Environment> globals;
        // current scope being executed
        std::shared_ptr<Environment> environment;

    public:
        Interpreter() {
            globals = std::make_shared<Environment>();
            environment = globals;
        }

        // entry point
        void interpret(const std::vector<std::unique_ptr<Stmt> > &statements);

    private:
        void execute(Stmt *stmt);

        Value evaluate(Expr *expr);

        // statement exec
        void execute_expression_stmt(const ExpressionStmt *stmt);

        void execute_print_stmt(const PrintStmt *stmt);

        void execute_var_stmt(const VarStmt *stmt);

        void execute_block_stmt(const BlockStmt *stmt);

        void execute_if_stmt(const IfStmt *stmt);

        void execute_while_stmt(const WhileStmt *stmt);

        void execute_break_stmt(const BreakStmt *stmt);

        void execute_return_stmt(const ReturnStmt *stmt);

        // expression eval
        Value evaluate_literal(LiteralExpr *expr);

        Value evaluate_variable(const VariableExpr *expr);

        Value evaluate_binary(const BinaryExpr *expr);

        Value evaluate_grouping(const GroupingExpr *expr);

        Value evaluate_unary(const UnaryExpr *expr);

        Value evaluate_update(const UpdateExpr *expr);

        Value evaluate_assignment(const AssignmentExpr *expr);

        Value evaluate_logical(const LogicalExpr *expr);

        // helpers
        bool is_truthy(const Value &value);

        void check_number_operand(const Token &oprt, const Value &oprnd);

        void check_number_operands(const Token &oprt, const Value &lhs, const Value &rhs);

        bool is_equal(const Value &a, const Value &b) const;

        void execute_block(const std::vector<
                               std::unique_ptr<Stmt> > &statements,
                           std::shared_ptr<Environment> block_env);
    };
}

#endif //OBLIBERRY_INTERPRETER_H
