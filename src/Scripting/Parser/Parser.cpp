#include "Parser.h"
#include "../Interpreter/Interpreter.h"
#include <charconv>
#include <format>

namespace ObSL {
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
        if (match({TokenType::USING})) return parse_using_statement();
        if (match({TokenType::FN})) return parse_function();
        if (match({TokenType::VAR})) return parse_var_statement();
        if (match({TokenType::TRY})) return parse_try_statement();
        if (match({TokenType::LEFT_BRACE})) return parse_block();
        if (match({TokenType::IF})) return parse_if_statement();
        if (match({TokenType::SWITCH})) return parse_switch_statement();
        if (match({TokenType::PRINT})) {
            Token keyword = previous();
            auto value = parse_expression();
            consume(TokenType::SEMICOLON, "Expect ';' after value.");
            return std::make_unique<PrintStmt>(keyword, std::move(value));
        }
        if (match({TokenType::PRINTLN})) {
            Token keyword = previous();
            auto value = parse_expression();
            consume(TokenType::SEMICOLON, "Expect ';' after value.");
            return std::make_unique<PrintlnStmt>(keyword, std::move(value));
        }
        if (match({TokenType::FOR})) return parse_for_statement();
        if (match({TokenType::FOREACH})) return parse_foreach_statement();
        if (match({TokenType::WHILE})) return parse_while_statement();
        if (match({TokenType::RETURN})) return parse_return_statement();
        if (match({TokenType::BREAK})) return parse_break_statement();
        auto expr = parse_expression();
        consume(TokenType::SEMICOLON, "Expect ';' after expression.");
        return std::make_unique<ExpressionStmt>(std::move(expr));
    }

    std::unique_ptr<Expr> Parser::parse_expression() {
        return parse_assignment();
    }

    std::unique_ptr<Expr> Parser::parse_assignment() {
        auto expr = parse_logical_or();
        if (match({
            TokenType::ASSIGN, TokenType::PLUS_EQUAL, TokenType::MINUS_EQUAL,
            TokenType::STAR_EQUAL, TokenType::SLASH_EQUAL, TokenType::PERCENT_EQUAL
        })) {
            const Token equals = previous();
            auto value = parse_assignment();
            if (const auto *var_expr = dynamic_cast<VariableExpr *>(expr.get())) {
                std::string_view name = var_expr->name;
                if (equals.type != TokenType::ASSIGN) {
                    TokenType binary_op = {};
                    std::string_view lexeme;
                    if (equals.type == TokenType::PLUS_EQUAL) {
                        binary_op = TokenType::PLUS;
                        lexeme = "+";
                    } else if (equals.type == TokenType::MINUS_EQUAL) {
                        binary_op = TokenType::MINUS;
                        lexeme = "-";
                    } else if (equals.type == TokenType::STAR_EQUAL) {
                        binary_op = TokenType::STAR;
                        lexeme = "*";
                    } else if (equals.type == TokenType::SLASH_EQUAL) {
                        binary_op = TokenType::SLASH;
                        lexeme = "/";
                    } else if (equals.type == TokenType::PERCENT_EQUAL) {
                        binary_op = TokenType::PERCENT;
                        lexeme = "%";
                    } else if (equals.type == TokenType::AMPERSAND) {
                        binary_op = TokenType::AMPERSAND;
                        lexeme = "&";
                    } else if (equals.type == TokenType::PIPE) {
                        binary_op = TokenType::PIPE;
                        lexeme = "|";
                    } else if (equals.type == TokenType::CARET) {
                        binary_op = TokenType::CARET;
                        lexeme = "^";
                    } else if (equals.type == TokenType::LESS_LESS) {
                        binary_op = TokenType::LESS_LESS;
                        lexeme = "<<";
                    } else if (equals.type == TokenType::GREATER_GREATER) {
                        binary_op = TokenType::GREATER_GREATER;
                        lexeme = ">>";
                    }
                    Token op_token = {
                        binary_op, lexeme, equals.line, equals.column, equals.start_pos, equals.end_pos
                    };
                    auto left_var = std::make_unique<VariableExpr>(name);
                    value = std::make_unique<BinaryExpr>(std::move(left_var), op_token, std::move(value));
                }
                return std::make_unique<AssignmentExpr>(name, std::move(value));
            }
            if (auto *index_expr = dynamic_cast<IndexExpr *>(expr.get())) {
                if (equals.type != TokenType::ASSIGN) {
                    throw RuntimeError(equals, "Compound assignment (+=, -=, etc) on arrays is not supported.");
                }
                auto callee = std::move(index_expr->callee);
                Token bracket = index_expr->bracket;
                auto index = std::move(index_expr->index);

                return std::make_unique<IndexAssignmentExpr>(
                    std::move(callee), bracket, std::move(index), std::move(value)
                );
            }
            if (auto *get_expr = dynamic_cast<GetExpr *>(expr.get())) {
                if (equals.type != TokenType::ASSIGN) {
                    throw RuntimeError(equals, "Compound assignment on object properties is not supported.");
                }
                return std::make_unique<SetExpr>(std::move(get_expr->obj), get_expr->name, std::move(value));
            }
            throw RuntimeError(equals, "Invalid assignment target.");
        }
        return expr;
    }


    std::unique_ptr<Stmt> Parser::parse_function() {
        Token name = consume(TokenType::IDENTIFIER, "Expect function name.");
        consume(TokenType::LEFT_PAREN, "Expect '(' after function name.");
        std::vector<Param> params;
        if (!check(TokenType::RIGHT_PAREN)) {
            bool has_default = false;
            do {
                if (params.size() >= 255) {
                    throw RuntimeError(peek(), "Can't have more than 255 parameters.");
                }
                const Token param_name = consume(TokenType::IDENTIFIER, "Expect parameter name.");
                std::unique_ptr<Expr> default_value = nullptr;

                if (match(TokenType::ASSIGN)) {
                    has_default = true;
                    default_value = parse_expression();
                } else {
                    if (has_default) {
                        throw RuntimeError(param_name, "Mandatory parameter cannot follow an optional parameter.");
                    }
                }

                params.emplace_back(param_name, std::move(default_value));
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
        consume(TokenType::LEFT_BRACE, "Expect '{' before function body.");
        auto body = parse_block();
        return std::make_unique<FunctionStmt>(name, std::move(params), std::move(body));
    }

    std::unique_ptr<Expr> Parser::parse_call() {
        auto expr = parse_primary();
        while (true) {
            if (match({TokenType::LEFT_PAREN})) {
                std::vector<std::unique_ptr<Expr> > args;
                if (!check(TokenType::RIGHT_PAREN)) {
                    do {
                        args.push_back(parse_expression());
                    } while (match({TokenType::COMMA}));
                }
                Token paren = consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
                expr = std::make_unique<CallExpr>(std::move(expr), paren, std::move(args));
            } else if (match({TokenType::LEFT_BRACKET})) {
                Token bracket = previous();
                auto index = parse_expression();
                consume(TokenType::RIGHT_BRACKET, "Expect ']' after index.");
                expr = std::make_unique<IndexExpr>(std::move(expr), bracket, std::move(index));
            } else if (match({TokenType::DOT})) {
                Token name = consume(TokenType::IDENTIFIER, "Expect property name after '.'.");
                expr = std::make_unique<GetExpr>(std::move(expr), name);
            } else {
                break;
            }
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
        auto expr = parse_shift();
        while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL})) {
            Token op = previous();
            auto right = parse_shift();
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
        while (match({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT})) {
            Token op = previous();
            auto right = parse_unary();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::parse_logical_or() {
        auto expr = parse_logical_and();
        while (match({TokenType::OR})) {
            Token op = previous();
            auto right = parse_logical_and();
            expr = std::make_unique<LogicalExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::parse_logical_and() {
        auto expr = parse_bitwise_or();
        while (match({TokenType::AND})) {
            Token op = previous();
            auto right = parse_bitwise_or();
            expr = std::make_unique<LogicalExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::parse_unary() {
        if (match({TokenType::BANG, TokenType::MINUS, TokenType::TILDE})) {
            Token op = previous();
            auto right = parse_unary();
            return std::make_unique<UnaryExpr>(op, std::move(right));
        }

        if (match({TokenType::PLUS_PLUS, TokenType::MINUS_MINUS})) {
            Token op = previous();
            Token name = consume(TokenType::IDENTIFIER, "Expect variable name after prefix operator.");
            return std::make_unique<UpdateExpr>(name, op, true);
        }
        return parse_postfix();
    }

    std::unique_ptr<Expr> Parser::parse_postfix() {
        auto expr = parse_call();
        if (match({TokenType::PLUS_PLUS, TokenType::MINUS_MINUS})) {
            Token op = previous();
            if (const auto *var_expr = dynamic_cast<VariableExpr *>(expr.get())) {
                return std::make_unique<UpdateExpr>(var_expr->name, op, false);
            }
            throw RuntimeError(op, "Invalid target for postfix operator.");
        }
        return expr;
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
                throw RuntimeError(tok, "Failed to parse number.");
            }
            return std::make_unique<LiteralExpr>(tok, Value(num));
        }

        if (match({TokenType::STRING})) {
            Token tok = previous();
            std::string processed_string;
            processed_string.reserve(tok.lexeme.size() > 2 ? tok.lexeme.size() - 2 : 0);
            for (size_t i = 1; i < tok.lexeme.size() - 1; ++i) {
                if (tok.lexeme[i] == '\\' && i + 1 < tok.lexeme.size() - 1) {
                    switch (const char next = tok.lexeme[i + 1]) {
                        case 'n': processed_string += '\n';
                            break;
                        case 't': processed_string += '\t';
                            break;
                        case 'r': processed_string += '\r';
                            break;
                        case '0': processed_string += '\0';
                            break;
                        case 'b': processed_string += '\b';
                            break;
                        case '\\': processed_string += '\\';
                            break;
                        case '"': processed_string += '"';
                            break;
                        case '\'': processed_string += '\'';
                            break;
                        default:
                            processed_string += '\\';
                            processed_string += next;
                            break;
                    }
                    ++i;
                } else {
                    processed_string += tok.lexeme[i];
                }
            }
            return std::make_unique<LiteralExpr>(tok, Value(processed_string));
        }

        if (match({TokenType::IDENTIFIER})) {
            return std::make_unique<VariableExpr>(previous());
        }

        if (match({TokenType::LEFT_PAREN})) {
            auto expr = parse_expression();
            consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
            return std::make_unique<GroupingExpr>(std::move(expr));
        }
        if (match({TokenType::LEFT_BRACKET})) {
            std::vector<std::unique_ptr<Expr> > elements;
            if (!check(TokenType::RIGHT_BRACKET)) {
                do {
                    elements.push_back(parse_expression());
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RIGHT_BRACKET, "Expect ']' after array elements");
            return std::make_unique<ArrayExpr>(std::move(elements));
        }

        throw RuntimeError(peek(), "Expect expression.");
    }

    std::unique_ptr<BlockStmt> Parser::parse_block() {
        std::vector<std::unique_ptr<Stmt> > stmts;
        while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
            stmts.push_back(parse_statement());
        }
        consume(TokenType::RIGHT_BRACE, "Expect '}' to close block.");
        return std::make_unique<BlockStmt>(std::move(stmts));
    }

    std::unique_ptr<Stmt> Parser::parse_using_statement() {
        Token keyword = previous();
        const Token path_token = consume(TokenType::STRING, "Expected string literal for filepath");
        consume(TokenType::SEMICOLON, "Expect ';' after using statement");

        std::string processed_path;
        processed_path.reserve(path_token.lexeme.size() > 2 ? path_token.lexeme.size() - 2 : 0);

        for (size_t i = 1; i < path_token.lexeme.size() - 1; ++i) {
            if (path_token.lexeme[i] == '\\' && i + 1 < path_token.lexeme.size() - 1) {
                switch (const char next = path_token.lexeme[i + 1]) {
                    case 'n': processed_path += '\n';
                        break;
                    case 't': processed_path += '\t';
                        break;
                    case 'r': processed_path += '\r';
                        break;
                    case '\\': processed_path += '\\';
                        break;
                    case '"': processed_path += '"';
                        break;
                    case '\'': processed_path += '\'';
                        break;
                    default:
                        processed_path += '\\';
                        processed_path += next;
                        break;
                }
                ++i;
            } else {
                processed_path += path_token.lexeme[i];
            }
        }
        return std::make_unique<UsingStmt>(keyword, std::move(processed_path));
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
        return std::make_unique<IfStmt>(std::move(condition), std::move(then_branch), std::move(else_branch));
    }

    std::unique_ptr<Stmt> Parser::parse_while_statement() {
        consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
        auto condition = parse_expression();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after while condition.");
        auto body = parse_statement();
        return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    }

    std::unique_ptr<Stmt> Parser::parse_foreach_statement() {
        consume(TokenType::LEFT_PAREN, "Expect '(' after 'foreach'.");
        match({TokenType::VAR});
        Token loop_var = consume(TokenType::IDENTIFIER, "Expect variable name.");
        consume(TokenType::IN, "Expect 'in' after variable name.");
        auto iterable = parse_expression();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after foreach clauses.");
        auto body = parse_statement();
        return std::make_unique<ForeachStmt>(loop_var, std::move(iterable), std::move(body));
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

    std::unique_ptr<Stmt> Parser::parse_break_statement() {
        Token keyword = previous();
        consume(TokenType::SEMICOLON, "Expect ';' after 'break'.");
        return std::make_unique<BreakStmt>(keyword);
    }

    std::unique_ptr<Stmt> Parser::parse_switch_statement() {
        consume(TokenType::LEFT_PAREN, "Expect '(' after 'switch'.");
        auto condition = parse_expression();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after switch condition.");
        consume(TokenType::LEFT_BRACE, "Expect '{' before switch cases.");

        std::vector<CaseBranch> cases;
        while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
            std::unique_ptr<Expr> match_value = nullptr;
            if (match({TokenType::CASE})) {
                match_value = parse_expression();
                consume(TokenType::COLON, "Expect ':' after case value.");
            } else if (match({TokenType::DEFAULT})) {
                consume(TokenType::COLON, "Expect ':' after default.");
            } else {
                throw RuntimeError(peek(), "Expect 'case' or 'default'.");
            }

            std::vector<std::unique_ptr<Stmt> > statements;
            while (!check(TokenType::CASE) && !check(TokenType::DEFAULT) && !check(TokenType::RIGHT_BRACE) && !
                   is_at_end()) {
                statements.push_back(parse_statement());
            }
            cases.emplace_back(std::move(match_value), std::move(statements));
        }
        consume(TokenType::RIGHT_BRACE, "Expect '}' to close switch block.");
        return std::make_unique<SwitchStmt>(std::move(condition), std::move(cases));
    }

    std::unique_ptr<Stmt> Parser::parse_var_statement() {
        Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");
        std::unique_ptr<Expr> initializer = nullptr;
        if (match({TokenType::ASSIGN})) {
            initializer = parse_expression();
        }
        consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
        return std::make_unique<VarStmt>(name, std::move(initializer));
    }

    std::unique_ptr<Stmt> Parser::parse_for_statement() {
        consume(TokenType::LEFT_PAREN, "Expect '(' after 'for'.");

        std::unique_ptr<Stmt> initializer;
        if (match({TokenType::SEMICOLON})) {
            initializer = nullptr;
        } else if (match({TokenType::VAR})) {
            initializer = parse_var_statement();
        } else {
            auto expr = parse_expression();
            consume(TokenType::SEMICOLON, "Expect ';' after loop initializer.");
            initializer = std::make_unique<ExpressionStmt>(std::move(expr));
        }

        std::unique_ptr<Expr> condition = nullptr;
        if (!check(TokenType::SEMICOLON)) {
            condition = parse_expression();
        }
        consume(TokenType::SEMICOLON, "Expect ';' after loop condition");

        std::unique_ptr<Expr> increment = nullptr;
        if (!check(TokenType::RIGHT_PAREN)) {
            increment = parse_expression();
        }
        consume(TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");

        std::unique_ptr<Stmt> body = parse_statement();

        if (increment != nullptr) {
            std::vector<std::unique_ptr<Stmt> > body_stmts;
            body_stmts.push_back(std::move(body));
            body_stmts.push_back(std::make_unique<ExpressionStmt>(std::move(increment)));
            body = std::make_unique<BlockStmt>(std::move(body_stmts));
        }

        if (condition == nullptr) {
            Token true_token{TokenType::TRUE_, "true", previous().line, previous().column, 0, 0};
            condition = std::make_unique<LiteralExpr>(true_token, Value(true));
        }

        body = std::make_unique<WhileStmt>(std::move(condition), std::move(body));

        if (initializer != nullptr) {
            std::vector<std::unique_ptr<Stmt> > for_stmts;
            for_stmts.push_back(std::move(initializer));
            for_stmts.push_back(std::move(body));
            body = std::make_unique<BlockStmt>(std::move(for_stmts));
        }
        return body;
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
        if (is_at_end() || current + 1 >= tokens.size()) return false;
        return tokens[current + 1].type == type;
    }

    bool Parser::is_at_end() const {
        if (current >= tokens.size()) return true;
        return peek().type == TokenType::EOF_;
    }

    Token Parser::consume(const TokenType type, const std::string_view message) {
        if (check(type)) return advance();
        throw RuntimeError(peek(), std::string(message));
    }

    std::unique_ptr<Expr> Parser::parse_bitwise_or() {
        auto expr = parse_bitwise_xor();
        while (match({TokenType::PIPE})) {
            Token op = previous();
            auto right = parse_bitwise_xor();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::parse_bitwise_xor() {
        auto expr = parse_bitwise_and();
        while (match({TokenType::CARET})) {
            Token op = previous();
            auto right = parse_bitwise_and();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::parse_bitwise_and() {
        auto expr = parse_equality();
        while (match({TokenType::AMPERSAND})) {
            Token op = previous();
            auto right = parse_equality();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::parse_shift() {
        auto expr = parse_term();
        while (match({TokenType::LESS_LESS, TokenType::GREATER_GREATER})) {
            Token op = previous();
            auto right = parse_term();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Stmt> Parser::parse_try_statement() {
        consume(TokenType::LEFT_BRACE, "Expect '{' before try block.");
        auto try_body = parse_block();
        consume(TokenType::CATCH, "Expect 'catch' after try block.");
        consume(TokenType::LEFT_PAREN, "Expect '(' after catch.");
        Token exception_var = consume(TokenType::IDENTIFIER, "Expect exception variable name.");
        consume(TokenType::RIGHT_PAREN, "Expect ')' after catch exception name.");
        consume(TokenType::LEFT_BRACE, "Expect '{' before catch block.");
        auto catch_body = parse_block();

        return std::make_unique<TryCatchStmt>(
            std::move(try_body),
            exception_var,
            std::move(catch_body)
        );
    }
} // namespace ObSL
