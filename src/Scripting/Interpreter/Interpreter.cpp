#include "Interpreter.h"

#include <cmath>
#include <iostream>

namespace ObSL {
    void Interpreter::interpret(const std::vector<std::unique_ptr<Stmt> > &statements) {
        try {
            for (const auto &stmt: statements) {
                if (stmt) { execute(stmt.get()); }
            }
        } catch (const RuntimeError &error) {
            std::cerr << std::format("Runtime Error at {} : {}\n",
                                     error.token.lexeme, error.what());
        }
    }

    //
    // dispathcers
    //

    void Interpreter::execute(Stmt *stmt) {
        if (const auto s = dynamic_cast<ExpressionStmt *>(stmt)) return execute_expression_stmt(s);
        if (const auto s = dynamic_cast<PrintStmt *>(stmt)) return execute_print_stmt(s);
        if (const auto s = dynamic_cast<VarStmt *>(stmt)) return execute_var_stmt(s);
        if (const auto s = dynamic_cast<BlockStmt *>(stmt)) return execute_block_stmt(s);
        if (const auto s = dynamic_cast<IfStmt *>(stmt)) return execute_if_stmt(s);
        if (const auto s = dynamic_cast<WhileStmt *>(stmt)) return execute_while_stmt(s);
        if (const auto s = dynamic_cast<BreakStmt *>(stmt)) return execute_break_stmt(s);
        if (const auto s = dynamic_cast<ReturnStmt *>(stmt)) return execute_return_stmt(s);
        throw std::runtime_error("Unknown statement type in interpreter.");
    }

    Value Interpreter::evaluate(Expr *expr) {
        if (const auto e = dynamic_cast<LiteralExpr *>(expr)) return evaluate_literal(e);
        if (const auto e = dynamic_cast<VariableExpr *>(expr)) return evaluate_variable(e);
        if (const auto e = dynamic_cast<BinaryExpr *>(expr)) return evaluate_binary(e);
        if (const auto e = dynamic_cast<GroupingExpr *>(expr)) return evaluate_grouping(e);
        if (const auto e = dynamic_cast<UnaryExpr *>(expr)) return evaluate_unary(e);
        if (const auto e = dynamic_cast<UpdateExpr *>(expr)) return evaluate_update(e);
        if (const auto e = dynamic_cast<AssignmentExpr *>(expr)) return evaluate_assignment(e);
        if (const auto e = dynamic_cast<LogicalExpr *>(expr)) return evaluate_logical(e);
        throw std::runtime_error("Unknown expression type in interpreter.");
    }


    //
    // statements
    //

    void Interpreter::execute_expression_stmt(const ExpressionStmt *stmt) {
        evaluate(stmt->expression.get());
    }

    void Interpreter::execute_print_stmt(const PrintStmt *stmt) {
        Value value = evaluate(stmt->expression.get());
        // ptrint via vistor
        std::visit([]<typename T0>(const T0 &arg) {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, std::monostate>) std::cout << "null\n";
            else if constexpr (std::is_same_v<T, bool>) std::cout << (arg ? "true\n" : "false\n");
            else if constexpr (std::is_same_v<T, double>) std::cout << arg << "\n";
            else if constexpr (std::is_same_v<T, std::string>) std::cout << arg << "\n";
        }, value);
    }

    void Interpreter::execute_var_stmt(const VarStmt *stmt) {
        Value value = std::monostate{};
        if (stmt->initializer) {
            value = evaluate(stmt->initializer.get());
        }
        environment->define(std::string(stmt->name.lexeme), value);
    }

    //
    // expressions
    //
    Value Interpreter::evaluate_literal(LiteralExpr *expr) {
        return expr->value;
    }

    Value Interpreter::evaluate_variable(const VariableExpr *expr) {
        return environment->get(expr->name);
    }

    Value Interpreter::evaluate_binary(const BinaryExpr *expr) {
        const Value lhs = evaluate(expr->left.get());
        const Value rhs = evaluate(expr->right.get());
        switch (expr->oprt.type) {
            //
            // comparisons
            //
            case TokenType::GREATER:
                check_number_operands(expr->oprt, lhs, rhs);
                return std::get<double>(lhs) > std::get<double>(rhs);
            case TokenType::GREATER_EQUAL:
                check_number_operands(expr->oprt, lhs, rhs);
                return std::get<double>(lhs) >= std::get<double>(rhs);
            case TokenType::LESS:
                check_number_operands(expr->oprt, lhs, rhs);
                return std::get<double>(lhs) < std::get<double>(rhs);
            case TokenType::LESS_EQUAL:
                check_number_operands(expr->oprt, lhs, rhs);
                return std::get<double>(lhs) <= std::get<double>(rhs);
            //
            // equality
            //
            case TokenType::BANG_EQUAL:
                return !is_equal(lhs, rhs);
            case TokenType::EQUAL_EQUAL:
                return is_equal(lhs, rhs);

            //
            // maths
            //
            case TokenType::MINUS:
                check_number_operands(expr->oprt, lhs, rhs);
                return std::get<double>(lhs) - std::get<double>(rhs);
            // plus is used for both maths and concatenation
            case TokenType::PLUS:
                if (std::holds_alternative<double>(lhs) && std::holds_alternative<double>(rhs)) {
                    return std::get<double>(lhs) + std::get<double>(rhs);
                }
                if (std::holds_alternative<std::string>(lhs) && std::holds_alternative<std::string>(rhs)) {
                    return std::get<std::string>(lhs) + std::get<std::string>(rhs);
                }
                throw RuntimeError(expr->oprt,
                                   "Operands must be two numbers or two strings.");
            case TokenType::SLASH:
                check_number_operands(expr->oprt, lhs, rhs);
                if (std::get<double>(rhs) == 0) throw RuntimeError(expr->oprt, "Division by zero.");
                return std::get<double>(lhs) / std::get<double>(rhs);
            case TokenType::PERCENT:
                check_number_operands(expr->oprt, lhs, rhs);
                if (std::get<double>(rhs) == 0) throw RuntimeError(expr->oprt, "Modulo by zero.");
                return std::fmod(std::get<double>(lhs), std::get<double>(rhs));

            case TokenType::STAR:
                check_number_operands(expr->oprt, lhs, rhs);
                return std::get<double>(lhs) * std::get<double>(rhs);
            default:
                break;
        }
        return std::monostate{};
    }

    Value Interpreter::evaluate_grouping(const GroupingExpr *expr) {
        return evaluate(expr->expr.get());
    }

    Value Interpreter::evaluate_unary(const UnaryExpr *expr) {
        Value right = evaluate(expr->right.get());
        switch (expr->oprt.type) {
            case TokenType::MINUS:
                check_number_operand(expr->oprt, right);
                return -std::get<double>(right);
            case TokenType::BANG:
                return !is_truthy(right);
            default:
                break;
        }
        return std::monostate{};
    }

    Value Interpreter::evaluate_update(const UpdateExpr *expr) {
        Value current_value = environment->get(expr->name);
        check_number_operand(expr->oprt, current_value);
        double num = std::get<double>(current_value);
        double new_num = (
                             expr->oprt.type ==
                             TokenType::PLUS_PLUS)
                             ? num + 1.0
                             : num - 1.0;
        environment->assign(expr->name, Value(new_num));
        return expr->is_prefix ? Value(new_num) : Value(num);
    }

    Value Interpreter::evaluate_assignment(const AssignmentExpr *expr) {
        Value value = evaluate(expr->value.get());
        environment->assign(expr->name, value);
        return value;
    }

    Value Interpreter::evaluate_logical(const LogicalExpr *expr) {
        Value left = evaluate(expr->left.get());
        // or returns left if truthy, and returns left if falsy
        if (expr->oprt.type == TokenType::OR) {
            if (is_truthy(left)) return left;
        } else {
            if (!is_truthy(left)) return left;
        }
        return evaluate(expr->right.get());
    }


    bool Interpreter::is_truthy(const Value &value) {
        if (std::holds_alternative<std::monostate>(value))return false;
        if (std::holds_alternative<bool>(value))return std::get<bool>(value);
        // other values are true since they are existing
        return true;
    }

    void Interpreter::check_number_operand(const Token &oprt, const Value &oprnd) {
        if (std::holds_alternative<double>(oprnd)) {
            return;
        }
        throw RuntimeError(oprt, "Operand must be a number.");
    }

    void Interpreter::check_number_operands(const Token &oprt, const Value &lhs, const Value &rhs) {
        if (!std::holds_alternative<double>(lhs) ||
            !std::holds_alternative<double>(rhs)) {
            throw RuntimeError(oprt, "Operands must be numbers.");
        }
    }

    bool Interpreter::is_equal(const Value &a, const Value &b) const {
        return std::visit([]<typename T0, typename T1>(const T0 &lhs, const T1 &rhs) -> bool {
            using L = std::decay_t<T0>;
            using R = std::decay_t<T1>;
            if constexpr (std::is_same_v<L, R>) {
                return lhs == rhs;
            } else {
                return false;
            }
        }, a, b);
    }

    void Interpreter::execute_block(const std::vector<std::unique_ptr<Stmt> > &statements,
                                    std::shared_ptr<Environment> block_env) {
        auto previous_env = environment;
        environment = std::move(block_env);
        try {
            for (const auto &stmt: statements) {
                if (stmt) execute(stmt.get());
            }
        } catch (...) {
            environment = previous_env;
            throw;
        }
        environment = previous_env;
    }

    void Interpreter::execute_block_stmt(const BlockStmt *stmt) {
        auto block_env = std::make_shared<Environment>(environment);
        execute_block(stmt->statements, block_env);
    }

    void Interpreter::execute_if_stmt(const IfStmt *stmt) {
        if (is_truthy(evaluate(stmt->condition.get()))) {
            execute(stmt->then_branch.get());
        } else if (stmt->else_branch) {
            execute(stmt->else_branch.get());
        }
    }

    void Interpreter::execute_while_stmt(const WhileStmt *stmt) {
        while (is_truthy(evaluate(stmt->condition.get()))) {
            try {
                execute(stmt->body.get());
            } catch (const BreakException &) {
                break;
            }
        }
    }

    void Interpreter::execute_break_stmt(const BreakStmt *) {
        throw BreakException();
    }

    void Interpreter::execute_return_stmt(const ReturnStmt *stmt) {
        // TODO: implement return value propagation when functions are added
        throw std::runtime_error("Return statement outside of function.");
    }
}
