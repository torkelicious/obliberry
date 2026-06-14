#ifndef OBLIBERRY_AST_H
#define OBLIBERRY_AST_H

#include <memory>
#include <utility>
#include <variant>
#include <string_view>
#include <string>
#include <vector>
#include <format>
#include "Scripting/Lexer/Lexer.h"

namespace ObSL {
    using Value = std::variant<
        std::monostate, // Represents Null
        bool,
        double,
        std::string
    >;

    // Base AST nodes
    struct Expr {
        virtual ~Expr() = default;

        [[nodiscard]] virtual std::string to_string() const = 0;
    };

    struct Stmt {
        virtual ~Stmt() = default;

        [[nodiscard]] virtual std::string to_string() const = 0;
    };

    //
    // Expressions
    //

    struct LiteralExpr : public Expr {
        Token token;
        Value value;

        explicit LiteralExpr(const Token &token, Value value)
            : token(token), value(std::move(value)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::visit([]<typename T0>(const T0 &arg) -> std::string {
                using T = std::decay_t<T0>;
                if constexpr (std::is_same_v<T, std::monostate>) return "null";
                else if constexpr (std::is_same_v<T, bool>) return arg ? std::string("true") : std::string("false");
                else if constexpr (std::is_same_v<T, double>) return std::format("{}", arg);
                else if constexpr (std::is_same_v<T, std::string>) return std::format("\"{}\"", arg);
            }, value);
        }
    };

    struct BinaryExpr : public Expr {
        std::unique_ptr<Expr> left;
        Token oprt;
        std::unique_ptr<Expr> right;

        BinaryExpr(std::unique_ptr<Expr> left, const Token &oprt, std::unique_ptr<Expr> right)
            : left(std::move(left)), oprt(oprt), right(std::move(right)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("({} {} {})", oprt.lexeme, left->to_string(), right->to_string());
        }
    };

    struct LogicalExpr : public Expr {
        std::unique_ptr<Expr> left;
        Token oprt;
        std::unique_ptr<Expr> right;

        LogicalExpr(std::unique_ptr<Expr> left, const Token &oprt, std::unique_ptr<Expr> right)
            : left(std::move(left)), oprt(oprt), right(std::move(right)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("({} {} {})", oprt.lexeme, left->to_string(), right->to_string());
        }
    };

    struct GroupingExpr : public Expr {
        std::unique_ptr<Expr> expr;

        explicit GroupingExpr(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("(group {})", expr->to_string());
        }
    };

    struct UnaryExpr : public Expr {
        Token oprt;
        std::unique_ptr<Expr> right;

        UnaryExpr(const Token &oprt, std::unique_ptr<Expr> right)
            : oprt(oprt), right(std::move(right)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("({} {})", oprt.lexeme, right->to_string());
        }
    };

    struct VariableExpr : public Expr {
        Token name;

        explicit VariableExpr(const Token &name) : name(name) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("{}", name.lexeme);
        }
    };

    struct AssignmentExpr : public Expr {
        Token name;
        std::unique_ptr<Expr> value;

        AssignmentExpr(const Token &name, std::unique_ptr<Expr> value)
            : name(name), value(std::move(value)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("(= {} {})", name.lexeme, value->to_string());
        }
    };

    struct UpdateExpr : public Expr {
        Token name;
        Token oprt;
        bool is_prefix;

        UpdateExpr(const Token &name, const Token &oprt, bool is_prefix)
            : name(name), oprt(oprt), is_prefix(is_prefix) {
        }

        [[nodiscard]] std::string to_string() const override {
            if (is_prefix) return std::format("({}{})", oprt.lexeme, name.lexeme);
            return std::format("({}{})", name.lexeme, oprt.lexeme);
        }
    };

    //
    // Statements
    //

    struct ExpressionStmt : public Stmt {
        std::unique_ptr<Expr> expression;

        explicit ExpressionStmt(std::unique_ptr<Expr> expr) : expression(std::move(expr)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[ExprStmt: {}]\n", expression->to_string());
        }
    };

    struct PrintStmt : public Stmt {
        Token keyword;
        std::unique_ptr<Expr> expression;

        PrintStmt(const Token &keyword, std::unique_ptr<Expr> expr)
            : keyword(keyword), expression(std::move(expr)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[PrintStmt: {}]\n", expression->to_string());
        }
    };

    struct BlockStmt : public Stmt {
        std::vector<std::unique_ptr<Stmt> > statements;

        explicit BlockStmt(std::vector<std::unique_ptr<Stmt> > stmts) : statements(std::move(stmts)) {
        }

        [[nodiscard]] std::string to_string() const override {
            std::string body;
            for (const auto &inner_stmt: statements) {
                body += std::format("  {}", inner_stmt->to_string());
            }
            return std::format("[BlockStmt: {{\n{}}}]\n", body);
        }
    };

    struct IfStmt : public Stmt {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> then_branch;
        std::unique_ptr<Stmt> else_branch;

        IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> then_branch, std::unique_ptr<Stmt> else_branch)
            : condition(std::move(condition)), then_branch(std::move(then_branch)),
              else_branch(std::move(else_branch)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[IfStmt: (if {}\n  then: {}  {})]\n",
                               condition->to_string(),
                               then_branch->to_string(),
                               else_branch ? std::format("else: {}", else_branch->to_string()) : ""
            );
        }
    };

    struct WhileStmt : public Stmt {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> body;

        WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
            : condition(std::move(condition)), body(std::move(body)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[WhileStmt: (while {}\n  body: {})]\n", condition->to_string(), body->to_string());
        }
    };

    struct ReturnStmt : public Stmt {
        Token keyword;
        std::unique_ptr<Expr> value;

        ReturnStmt(const Token &keyword, std::unique_ptr<Expr> value)
            : keyword(keyword), value(std::move(value)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[ReturnStmt: (return{})]\n", value ? std::format(" {}", value->to_string()) : "");
        }
    };

    struct BreakStmt : public Stmt {
        Token keyword;

        explicit BreakStmt(const Token &keyword) : keyword(keyword) {
        }

        [[nodiscard]] std::string to_string() const override {
            return "[BreakStmt]\n";
        }
    };

    struct VarStmt : public Stmt {
        Token name;
        std::unique_ptr<Expr> initializer;

        VarStmt(const Token &name, std::unique_ptr<Expr> initializer)
            : name(name), initializer(std::move(initializer)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[VarStmt: (var {}{})]\n",
                               name.lexeme,
                               initializer ? std::format(" = {}", initializer->to_string()) : ""
            );
        }
    };
}

#endif //OBLIBERRY_AST_H
