#include "Lexer.h"
#include <cctype>
#include <iostream>

namespace Scripting {
    Lexer::Lexer(std::string_view source)
        : source(source) {
    }

    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> tokens;
        while (!is_at_end()) {
            skip_whitespace();
            if (is_at_end()) break;
            if (char c = peek(); isdigit(c)) {
                tokens.push_back(read_num());
            } else if (isalpha(c) || c == '_') {
                tokens.push_back(read_identifier_or_keyword());
            } else if (c == '"') {
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
        return (current + 1 >= source.size()) ? '\0' : source[current + 1];
    }

    char Lexer::advance() {
        char chr = peek();
        ++current;
        ++column;
        return chr;
    }

    bool Lexer::is_at_end() const {
        return current >= source.size();
    }

    void Lexer::skip_whitespace() {
        while (true) {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\r') {
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
        size_t number_start = current;
        size_t start_col = column;

        while (isdigit(peek())) { advance(); }

        auto lexeme = source.substr(number_start, current - number_start);
        return Token{
            TokenType::NUMBER, lexeme, line, static_cast<int>(start_col), static_cast<int>(number_start),
            static_cast<int>(current)
        };
    }

    Token Lexer::read_identifier_or_keyword() {
        size_t id_start = current;
        size_t start_col = column;

        while (isalnum(peek()) || peek() == '_') { advance(); }

        auto text = source.substr(id_start, current - id_start);
        if (text == "print")
            return Token{
                TokenType::PRINT, text, line, static_cast<int>(start_col), static_cast<int>(id_start),
                static_cast<int>(current)
            };
        if (text == "if")
            return Token{
                TokenType::IF, text, line, static_cast<int>(start_col), static_cast<int>(id_start),
                static_cast<int>(current)
            };
        if (text == "while")
            return Token{
                TokenType::WHILE, text, line, static_cast<int>(start_col), static_cast<int>(id_start),
                static_cast<int>(current)
            };

        return Token{
            TokenType::IDENTIFIER, text, line, static_cast<int>(start_col), static_cast<int>(id_start),
            static_cast<int>(current)
        };
    }

    Token Lexer::read_string() {
        size_t start_col = column;
        size_t str_start = current;
        advance();

        while (peek() != '"' && !is_at_end()) {
            if (peek() == '\n') {
                line++;
                column = 0;
            }
            advance();
        }

        if (is_at_end()) {
            std::cerr << "line: " << line << " unterminated string at column: " << column << "\n";
            return Token{
                TokenType::EOF_, "", line, static_cast<int>(start_col), static_cast<int>(str_start),
                static_cast<int>(current)
            };
        }
        advance();
        std::string_view value = source.substr(str_start + 1, current - str_start - 2);
        return Token{
            TokenType::STRING, value, line, static_cast<int>(start_col), static_cast<int>(str_start),
            static_cast<int>(current)
        };
    }

    Token Lexer::read_operator_or_symbol() {
        size_t start_col = column;
        size_t start_pos = current;
        switch (char c = advance()) {
            case '+': return Token{
                    TokenType::PLUS, "+", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '-': return Token{
                    TokenType::MINUS, "-", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '*': return Token{
                    TokenType::STAR, "*", line, static_cast<int>(start_col), static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
            case '/': return Token{
                    TokenType::SLASH, "/", line, static_cast<int>(start_col), static_cast<int>(start_pos),
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
                    static_cast<int>(start_pos),
                    static_cast<int>(current)
                };
        }
    }
}
