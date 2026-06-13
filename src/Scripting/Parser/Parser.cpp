#include "Parser.h"
#include <stdexcept>
#include <charconv>

namespace Scripting {
    Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {
    }

    std::vector<std::unique_ptr<Stmt> > Parser::parse() {
        std::vector<std::unique_ptr<Stmt> > statements;
        while (!is_at_end()) {
            statements.push_back(parse_statement());
        }
        return statements;
    }

    std::unique_ptr<Stmt> Parser::parse_statement() {
        if (match({TokenType::IF})) return parse_if_statement();
        if (match({TokenType::WHILE})) return parse_while_statement();
        if (match({TokenType::RETURN})) return parse_return_statement();

        if (match({TokenType::LEFT_BRACE})) return parse_block();

        if (match({TokenType::PRINT})) {
            Token keyword = previous();
            auto value = parse_expression();
            consume(TokenType::SEMICOLON, "Expect ';' after value.");
            return std::make_unique<PrintStmt>(keyword, std::move(value));
        }

        auto expr = parse_expression();
        consume(TokenType::SEMICOLON, "Expect ';' after expression.");
        return std::make_unique<ExpressionStmt>(std::move(expr));
    }

    std::unique_ptr<Expr> Parser::parse_expression() {
        return parse_assignment();
    }

    std::unique_ptr<Expr> Parser::parse_assignment() {
        auto expr = parse_equality();

        if (match({TokenType::ASSIGN})) {
            const Token equals = previous();
            auto value = parse_assignment(); // recursively parse the right side
            if (const auto *var_expr = dynamic_cast<VariableExpr *>(expr.get())) {
                Token name = var_expr->name;
                return std::make_unique<AssignmentExpr>(name, std::move(value));
            }
            throw std::runtime_error("[Line " + std::to_string(equals.line) + "] Invalid assignment target.");
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::parse_equality() {
        auto expr = parse_comparison();

        while (match({TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL})) {
            Token op = previous();
            auto right = parse_comparison();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::parse_comparison() {
        auto expr = parse_term();

        while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL})) {
            Token op = previous();
            auto right = parse_term();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::parse_term() {
        auto expr = parse_factor();

        while (match({TokenType::PLUS, TokenType::MINUS})) {
            Token op = previous();
            auto right = parse_factor();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::parse_factor() {
        auto expr = parse_unary();

        while (match({TokenType::STAR, TokenType::SLASH})) {
            Token op = previous();
            auto right = parse_unary();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::parse_unary() {
        if (match({TokenType::BANG, TokenType::MINUS})) {
            Token op = previous();
            auto right = parse_unary();
            return std::make_unique<UnaryExpr>(op, std::move(right));
        }
        return parse_primary();
    }

    std::unique_ptr<Expr> Parser::parse_primary() {
        if (match({TokenType::FALSE_})) return std::make_unique<LiteralExpr>(previous(), Value(false));
        if (match({TokenType::TRUE_})) return std::make_unique<LiteralExpr>(previous(), Value(true));
        if (match({TokenType::NULL_})) return std::make_unique<LiteralExpr>(previous(), Value(std::monostate{}));

        if (match({TokenType::NUMBER})) {
            Token tok = previous();
            double num = 0.0;
            if (auto [ptr, ec] = std::from_chars(tok.lexeme.data(), tok.lexeme.data() + tok.lexeme.size(), num);
                ec != std::errc()) {
                throw std::runtime_error("Failed to parse number.");
            }
            return std::make_unique<LiteralExpr>(tok, Value(num));
        }

        if (match({TokenType::STRING})) {
            return std::make_unique<LiteralExpr>(previous(), Value(previous().lexeme));
        }

        if (match({TokenType::IDENTIFIER})) {
            return std::make_unique<VariableExpr>(previous());
        }

        if (match({TokenType::LEFT_PAREN})) {
            auto expr = parse_expression();
            consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
            return std::make_unique<GroupingExpr>(std::move(expr));
        }
        throw std::runtime_error("Expect expression.");
    }

    std::unique_ptr<Stmt> Parser::parse_block() {
        std::vector<std::unique_ptr<Stmt> > stmts;
        while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
            stmts.push_back(parse_statement());
        }
        consume(TokenType::RIGHT_BRACE, "Expect '}' to close block.");
        return std::make_unique<BlockStmt>(std::move(stmts));
    }

    std::unique_ptr<Stmt> Parser::parse_if_statement() {
        consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
        auto condition = parse_expression();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after if condition.");
        auto then_branch = parse_statement();
        std::unique_ptr<Stmt> else_branch = nullptr;
        if (match({TokenType::ELSE})) {
            else_branch = parse_statement();
        }
        return std::make_unique<IfStmt>(
            std::move(condition),
            std::move(then_branch),
            std::move(else_branch)
        );
    }

    std::unique_ptr<Stmt> Parser::parse_while_statement() {
        consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
        auto condition = parse_expression();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after while condition.");
        auto body = parse_statement();
        return std::make_unique<WhileStmt>(
            std::move(condition),
            std::move(body)
        );
    }

    std::unique_ptr<Stmt> Parser::parse_return_statement() {
        Token keyword = previous();
        std::unique_ptr<Expr> value = nullptr;
        if (!check(TokenType::SEMICOLON)) {
            value = parse_expression();
        }
        consume(TokenType::SEMICOLON, "Expect ';' after return value.");
        return std::make_unique<ReturnStmt>(keyword, std::move(value));
    }

    Token Parser::peek() const {
        return tokens[current];
    }

    Token Parser::previous() const {
        return tokens[current - 1];
    }

    Token Parser::advance() {
        if (!is_at_end()) ++current;
        return previous();
    }

    bool Parser::match(const std::initializer_list<TokenType> types) {
        for (const TokenType type: types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }

    bool Parser::match(const TokenType type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }

    bool Parser::check(const TokenType type) const {
        if (is_at_end()) return false;
        return peek().type == type;
    }

    bool Parser::check_next(const TokenType type) const {
        if (is_at_end()) return false;
        if (current + 1 >= tokens.size()) return false;
        return tokens[current + 1].type == type;
    }

    bool Parser::is_at_end() const {
        if (current >= tokens.size()) return true;
        return peek().type == TokenType::EOF_;
    }

    Token Parser::consume(const TokenType type, const std::string &message) {
        if (check(type)) return advance();
        throw std::runtime_error("[Line " + std::to_string(peek().line) + "] " + message);
    }
}
