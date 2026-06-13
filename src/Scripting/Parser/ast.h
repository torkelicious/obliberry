#ifndef OBLIBERRY_AST_H
#define OBLIBERRY_AST_H

#include <memory>
#include <variant>
#include <string_view>
#include "Scripting/Lexer/Lexer.h"

namespace Scripting {
    using Value = std::variant<
        std::monostate, // Represents Null / Nil / Void
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

        explicit LiteralExpr(Token token, Value value)
            : token(token), value(value) {
        }
    };

    struct BinaryExpr : public Expr {
        std::unique_ptr<Expr> left;
        Token oprt;
        std::unique_ptr<Expr> right;

        BinaryExpr(std::unique_ptr<Expr> left, Token oprt, std::unique_ptr<Expr> right)
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

        UnaryExpr(Token oprt, std::unique_ptr<Expr> right)
            : oprt(oprt), right(std::move(right)) {
        }
    };
}

#endif //OBLIBERRY_AST_H
