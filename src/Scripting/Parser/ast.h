#ifndef OBLIBERRY_AST_H
#define OBLIBERRY_AST_H
#include <memory>
#include <variant>
#include <string_view>
#include <vector>
#include "Scripting/Lexer/Lexer.h"

//
// todo:
// for loops
// variable declarations (var keyword)
// logical operators (and / or)
// function declarations and calls
// env & variable scoping
// the interpreter / evaluator (executing AST)
//


namespace Scripting {
    using Value = std::variant<
        std::monostate, // Represents Null / Nil / Void (im probably going to use "null")
        bool, // true / false
        double, // Numbers
        std::string_view // strings
    >;

    // base AST node
    struct Expr {
        virtual ~Expr() = default;
    };

    struct LiteralExpr : public Expr {
        Token token;
        Value value;

        explicit LiteralExpr(const Token &token, const Value &value)
            : token(token), value(value) {
        }
    };

    struct BinaryExpr : public Expr {
        std::unique_ptr<Expr> left;
        Token oprt;
        std::unique_ptr<Expr> right;

        BinaryExpr(std::unique_ptr<Expr> left, const Token &oprt, std::unique_ptr<Expr> right)
            : left(std::move(left)), oprt(oprt), right(std::move(right)) {
        }
    };

    struct GroupingExpr : public Expr {
        std::unique_ptr<Expr> expr;

        explicit GroupingExpr(std::unique_ptr<Expr> expr)
            : expr(std::move(expr)) {
        }
    };

    struct UnaryExpr : public Expr {
        Token oprt;
        std::unique_ptr<Expr> right;

        UnaryExpr(const Token &oprt, std::unique_ptr<Expr> right)
            : oprt(oprt), right(std::move(right)) {
        }
    };

    struct VariableExpr : public Expr {
        Token name;

        explicit VariableExpr(const Token &name)
            : name(name) {
        }
    };

    struct AssignmentExpr : public Expr {
        Token name;
        std::unique_ptr<Expr> value;

        AssignmentExpr(const Token &name, std::unique_ptr<Expr> value)
            : name(name), value(std::move(value)) {
        }
    };

    //
    // statements
    //

    struct Stmt {
        virtual ~Stmt() = default;
    };

    struct ExpressionStmt : public Stmt {
        std::unique_ptr<Expr> expression;

        explicit ExpressionStmt(std::unique_ptr<Expr> expr)
            : expression(std::move(expr)) {
        }
    };

    struct PrintStmt : public Stmt {
        Token keyword;
        std::unique_ptr<Expr> expression;

        PrintStmt(const Token &keyword, std::unique_ptr<Expr> expr)
            : keyword(keyword), expression(std::move(expr)) {
        }
    };

    struct BlockStmt : public Stmt {
        std::vector<std::unique_ptr<Stmt> > statements;

        explicit BlockStmt(std::vector<std::unique_ptr<Stmt> > stmts)
            : statements(std::move(stmts)) {
        }
    };

    struct IfStmt : public Stmt {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> then_branch;
        std::unique_ptr<Stmt> else_branch;

        IfStmt(std::unique_ptr<Expr> condition,
               std::unique_ptr<Stmt> then_branch,
               std::unique_ptr<Stmt> else_branch)
            : condition(std::move(condition)), then_branch(std::move(then_branch)),
              else_branch(std::move(else_branch)) {
        }
    };

    struct WhileStmt : public Stmt {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> body;

        WhileStmt(std::unique_ptr<Expr> condition,
                  std::unique_ptr<Stmt> body)
            : condition(std::move(condition)),
              body(std::move(body)) {
        }
    };

    struct ReturnStmt : public Stmt {
        Token keyword;
        std::unique_ptr<Expr> value; // can be empty i.e nullptr
        ReturnStmt(const Token &keyword,
                   std::unique_ptr<Expr> value)
            : keyword(keyword), value(std::move(value)) {
        }
    };
}

#endif //OBLIBERRY_AST_H
