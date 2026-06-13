#ifndef OBLIBERRY_PARSER_H
#define OBLIBERRY_PARSER_H
#include <vector>

#include "ast.h"
#include "Scripting/Lexer/Lexer.h"

namespace Scripting {
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

        Token consume(TokenType type, const std::string &message);

        std::unique_ptr<Stmt> parse_statement();

        std::unique_ptr<Expr> parse_expression();

        std::unique_ptr<Expr> parse_assignment();

        std::unique_ptr<Expr> parse_equality();

        std::unique_ptr<Expr> parse_comparison();

        std::unique_ptr<Expr> parse_unary();

        std::unique_ptr<Expr> parse_primary();

        std::unique_ptr<Expr> parse_term();

        std::unique_ptr<Expr> parse_factor();

        std::unique_ptr<Stmt> parse_block();

        std::unique_ptr<Stmt> parse_if_statement();

        std::unique_ptr<Stmt> parse_while_statement();

        std::unique_ptr<Stmt> parse_return_statement();
    };
}

#endif //OBLIBERRY_PARSER_H
