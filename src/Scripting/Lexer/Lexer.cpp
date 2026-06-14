#include "Lexer.h"
#include <cctype>
#include <iostream>
#include <unordered_map>
#include <format>

namespace ObSL {
    Lexer::Lexer(const std::string_view source) : source(source) {
    }

    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> tokens;
        while (!is_at_end()) {
            skip_whitespace();
            if (is_at_end()) break;
            if (const char c = peek(); std::isdigit(static_cast<unsigned char>(c))) {
                tokens.push_back(read_num());
            } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                tokens.push_back(read_identifier_or_keyword());
            } else if (c == '"' || c == '\'') {
                tokens.push_back(read_string());
            } else {
                tokens.push_back(read_operator_or_symbol());
            }
        }
        tokens.push_back(Token{
            TokenType::EOF_, "", line, column, static_cast<int>(current), static_cast<int>(current)
        });
        return tokens;
    }

    char Lexer::peek() const {
        return is_at_end() ? '\0' : source[current];
    }

    char Lexer::peek_next() const {
        return current + 1 >= source.size() ? '\0' : source[current + 1];
    }

    char Lexer::advance() {
        const char chr = peek();
        ++current;
        ++column;
        return chr;
    }

    bool Lexer::is_at_end() const {
        return current >= source.size();
    }

    void Lexer::skip_whitespace() {
        while (true) {
            if (const char c = peek(); c == ' ' || c == '\t' || c == '\r') {
                advance();
            } else if (c == '\n') {
                advance();
                ++line;
                column = 1;
            } else if (c == '/' && peek_next() == '/') {
                advance();
                advance();
                while (peek() != '\n' && !is_at_end()) {
                    advance();
                }
            } else {
                break;
            }
        }
    }

    Token Lexer::read_num() {
        const size_t number_start = current;
        const size_t start_col = column;
        while (std::isdigit(static_cast<unsigned char>(peek()))) { advance(); }
        // look for a fractional part.
        if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek_next()))) {
            advance(); // Consume the "."
            while (std::isdigit(static_cast<unsigned char>(peek()))) { advance(); }
        }
        const auto lexeme = source.substr(number_start, current - number_start);
        return Token{
            TokenType::NUMBER, lexeme, line, static_cast<int>(start_col), static_cast<int>(number_start),
            static_cast<int>(current)
        };
    }

    Token Lexer::read_identifier_or_keyword() {
        const size_t id_start = current;
        const size_t start_col = column;

        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') { advance(); }

        const auto text = source.substr(id_start, current - id_start);

        static const std::unordered_map<std::string_view, TokenType> keywords = {
            {"and", TokenType::AND},
            {"break", TokenType::BREAK},
            {"else", TokenType::ELSE},
            {"false", TokenType::FALSE_},
            {"fn", TokenType::FN},
            {"for", TokenType::FOR},
            {"if", TokenType::IF},
            {"null", TokenType::NULL_},
            {"or", TokenType::OR},
            {"print", TokenType::PRINT},
            {"println", TokenType::PRINTLN},
            {"return", TokenType::RETURN},
            {"this", TokenType::THIS},
            {"true", TokenType::TRUE_},
            {"var", TokenType::VAR},
            {"while", TokenType::WHILE}
        };

        const auto it = keywords.find(text);
        const TokenType type = it != keywords.end() ? it->second : TokenType::IDENTIFIER;

        return Token{
            type, text, line, static_cast<int>(start_col), static_cast<int>(id_start), static_cast<int>(current)
        };
    }

    Token Lexer::read_string() {
        const size_t start_col = column;
        const size_t str_start = current;
        // support ' or "
        const char quote_type = advance();
        while (peek() != quote_type && !is_at_end()) {
            if (peek() == '\n') {
                line++;
                column = 0;
            }
            advance();
        }
        if (is_at_end()) {
            std::cerr << std::format("line: {} unterminated string at column: {}\n", line, column);
            return Token{
                TokenType::EOF_, "", line, static_cast<int>(start_col), static_cast<int>(str_start),
                static_cast<int>(current)
            };
        }
        advance(); // consume the closing quote
        const std::string_view value = source.substr(str_start + 1, current - str_start - 2);
        return Token{
            TokenType::STRING, value, line, static_cast<int>(start_col), static_cast<int>(str_start),
            static_cast<int>(current)
        };
    }

    Token Lexer::read_operator_or_symbol() {
        const size_t start_col = column;
        const size_t start_pos = current;
        switch (advance()) {
            case '+':
                if (peek() == '+') {
                    advance();
                    return Token{
                        TokenType::PLUS_PLUS, "++", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                        static_cast<int>(current)
                    };
                }
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::PLUS_EQUAL, "+=", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                        static_cast<int>(current)
                    };
                }
                return Token{
                    TokenType::PLUS, "+", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '-':
                if (peek() == '-') {
                    advance();
                    return Token{
                        TokenType::MINUS_MINUS, "--", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                        static_cast<int>(current)
                    };
                }
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::MINUS_EQUAL, "-=", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                        static_cast<int>(current)
                    };
                }
                return Token{
                    TokenType::MINUS, "-", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '*':
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::STAR_EQUAL, "*=", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                        static_cast<int>(current)
                    };
                }
                return Token{
                    TokenType::STAR, "*", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '/':
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::SLASH_EQUAL, "/=", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                        static_cast<int>(current)
                    };
                }
                return Token{
                    TokenType::SLASH, "/", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '%':
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::PERCENT_EQUAL, "%=", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                        static_cast<int>(current)
                    };
                }
                return Token{
                    TokenType::PERCENT, "%", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '=':
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::EQUAL_EQUAL, "==", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                        static_cast<int>(current)
                    };
                }
                return Token{
                    TokenType::ASSIGN, "=", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '!':
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::BANG_EQUAL, "!=", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                        static_cast<int>(current)
                    };
                }
                return Token{
                    TokenType::BANG, "!", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '<':
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::LESS_EQUAL, "<=", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                        static_cast<int>(current)
                    };
                }
                return Token{
                    TokenType::LESS, "<", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '>':
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::GREATER_EQUAL, ">=", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                        static_cast<int>(current)
                    };
                }
                return Token{
                    TokenType::GREATER, ">", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '(': return Token{
                    TokenType::LEFT_PAREN, "(", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case ')': return Token{
                    TokenType::RIGHT_PAREN, ")", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '{': return Token{
                    TokenType::LEFT_BRACE, "{", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '}': return Token{
                    TokenType::RIGHT_BRACE, "}", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '[': return Token{
                    TokenType::LEFT_BRACKET, "[", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case ']': return Token{
                    TokenType::RIGHT_BRACKET, "]", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case ';': return Token{
                    TokenType::SEMICOLON, ";", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case ',': return Token{
                    TokenType::COMMA, ",", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '.': return Token{
                    TokenType::DOT, ".", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            default:
                return Token{
                    TokenType::UNKNOWN, std::string_view(&source[start_pos], 1), line, static_cast<int>(start_col),
                    static_cast<int>(start_pos), static_cast<int>(current)
                };
        }
    }
} // namespace ObSL
