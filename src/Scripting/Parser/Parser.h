#pragma once

#include <vector>
#include <memory>
#include <initializer_list>
#include <string_view>
#include "ast.h"
#include "Scripting/Lexer/Lexer.h"

namespace ObSL {
    class Parser {
    public:
        explicit Parser(std::vector<Token> tokens);

        std::vector<std::unique_ptr<Stmt> > parse();

    private:
        std::vector<Token> tokens;
        size_t current = 0;

        [[nodiscard]] Token peek() const;

        [[nodiscard]] Token previous() const;

        Token advance();

        bool match(std::initializer_list<TokenType> types);

        bool match(TokenType type);

        [[nodiscard]] bool check(TokenType type) const;

        [[nodiscard]] bool check_next(TokenType type) const;

        [[nodiscard]] bool is_at_end() const;

        Token consume(TokenType type, std::string_view message);


        std::unique_ptr<Stmt> parse_statement();

        std::unique_ptr<Stmt> parse_function();

        std::unique_ptr<Expr> parse_call();

        std::unique_ptr<Expr> parse_expression();

        std::unique_ptr<Expr> parse_assignment();

        std::unique_ptr<Expr> parse_equality();

        std::unique_ptr<Expr> parse_comparison();

        std::unique_ptr<Expr> parse_unary();

        std::unique_ptr<Expr> parse_postfix();

        std::unique_ptr<Expr> parse_primary();

        std::unique_ptr<Expr> parse_term();

        std::unique_ptr<Expr> parse_factor();

        std::unique_ptr<Expr> parse_logical_or();

        std::unique_ptr<Expr> parse_logical_and();

        std::unique_ptr<BlockStmt> parse_block();

        std::unique_ptr<Stmt> parse_if_statement();

        std::unique_ptr<Stmt> parse_while_statement();

        std::unique_ptr<Stmt> parse_return_statement();

        std::unique_ptr<Stmt> parse_break_statement();

        std::unique_ptr<Stmt> parse_switch_statement();

        std::unique_ptr<Stmt> parse_var_statement();

        std::unique_ptr<Stmt> parse_for_statement();
    };
} // namespace ObSL
