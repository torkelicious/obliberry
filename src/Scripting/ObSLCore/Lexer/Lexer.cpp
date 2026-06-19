#include "Lexer.h"
#include <cctype>
#include <unordered_map>
#include <format>
#include "Scripting/ObSLCore/Interpreter/Natives.h"

namespace ObSL {
    Lexer::Lexer(const std::string_view source) : source(source) {
    }

    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> tokens;

        if (source.length() >= 3 &&
            static_cast<unsigned char>(source[0]) == 0xEF &&
            static_cast<unsigned char>(source[1]) == 0xBB &&
            static_cast<unsigned char>(source[2]) == 0xBF) {
            current += 3;
        }

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
            TokenType::EOF_, "", static_cast<uint16_t>(line), static_cast<uint16_t>(column),
            static_cast<uint32_t>(current), static_cast<uint32_t>(current)
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

    // skips comments too
    void Lexer::skip_whitespace() {
        while (!is_at_end()) {
            if (const char c = peek(); c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f') {
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
            } else if (static_cast<unsigned char>(c) == 0xC2 && static_cast<unsigned char>(peek_next()) == 0xA0) {
                advance();
                advance();
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
            TokenType::NUMBER, lexeme, static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
            static_cast<uint32_t>(number_start),
            static_cast<uint32_t>(current)
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
            {"case", TokenType::CASE},
            {"catch", TokenType::CATCH},
            {"default", TokenType::DEFAULT},
            {"switch", TokenType::SWITCH},
            {"struct", TokenType::STRUCT},
            {"else", TokenType::ELSE},
            {"false", TokenType::FALSE_},
            {"fn", TokenType::FN},
            {"for", TokenType::FOR},
            {"foreach", TokenType::FOREACH},
            {"if", TokenType::IF},
            {"in", TokenType::IN},
            {"is", TokenType::IS},
            {"null", TokenType::NULL_},
            {"or", TokenType::OR},
            {"print", TokenType::PRINT},
            {"println", TokenType::PRINTLN},
            {"return", TokenType::RETURN},
            {"this", TokenType::THIS},
            {"true", TokenType::TRUE_},
            {"try", TokenType::TRY},
            {"using", TokenType::USING},
            {"var", TokenType::VAR},
            {"while", TokenType::WHILE},
        };

        const auto it = keywords.find(text);
        const TokenType type = it != keywords.end() ? it->second : TokenType::IDENTIFIER;

        return Token{
            type, text, static_cast<uint16_t>(line), static_cast<uint16_t>(start_col), static_cast<uint32_t>(id_start),
            static_cast<uint32_t>(current)
        };
    }

    Token Lexer::read_string() {
        const char quote_type = peek();
        const auto start_pos = static_cast<uint32_t>(current);
        const auto start_col = static_cast<uint16_t>(column);
        advance();
        while (!is_at_end() && peek() != quote_type) {
            if (peek() == '\n') {
                advance();
                line++;
                column = 1;
                continue;
            }
            if (peek() == '\\') {
                advance();
                if (!is_at_end()) {
                    if (peek() == '\n') {
                        advance();
                        line++;
                        column = 1;
                    } else {
                        advance();
                    }
                    continue;
                }
            }
            advance();
        }
        if (is_at_end()) {
            throw RuntimeError(Token{
                                   TokenType::UNKNOWN, "", static_cast<uint16_t>(line),
                                   static_cast<uint16_t>(start_col), static_cast<uint32_t>(current)
                               }, "Unterminated string.");
        }
        advance();
        return Token{
            TokenType::STRING,
            std::string_view(&source[start_pos], current - start_pos),
            static_cast<uint16_t>(line),
            static_cast<uint16_t>(start_col),
            static_cast<uint32_t>(current)
        };
    }

    Token Lexer::read_operator_or_symbol() {
        const uint16_t start_col = column;
        const uint32_t start_pos = current;
        switch (advance()) {
            case '+':
                if (peek() == '+') {
                    advance();
                    return Token{
                        TokenType::PLUS_PLUS, "++", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                        static_cast<uint32_t>(start_pos),
                        static_cast<uint32_t>(current)
                    };
                }
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::PLUS_EQUAL, "+=", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                        static_cast<uint32_t>(start_pos),
                        static_cast<uint32_t>(current)
                    };
                }
                return Token{
                    TokenType::PLUS, "+", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case '-':
                if (peek() == '-') {
                    advance();
                    return Token{
                        TokenType::MINUS_MINUS, "--", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                        static_cast<uint32_t>(start_pos),
                        static_cast<uint32_t>(current)
                    };
                }
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::MINUS_EQUAL, "-=", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                        static_cast<uint32_t>(start_pos),
                        static_cast<uint32_t>(current)
                    };
                }
                return Token{
                    TokenType::MINUS, "-", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case '*':
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::STAR_EQUAL, "*=", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                        static_cast<uint32_t>(start_pos),
                        static_cast<uint32_t>(current)
                    };
                }
                return Token{
                    TokenType::STAR, "*", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case '/':
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::SLASH_EQUAL, "/=", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                        static_cast<uint32_t>(start_pos),
                        static_cast<uint32_t>(current)
                    };
                }
                return Token{
                    TokenType::SLASH, "/", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case '%':
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::PERCENT_EQUAL, "%=", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                        static_cast<uint32_t>(start_pos),
                        static_cast<uint32_t>(current)
                    };
                }
                return Token{
                    TokenType::PERCENT, "%", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case '=':
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::EQUAL_EQUAL, "==", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                        static_cast<uint32_t>(start_pos),
                        static_cast<uint32_t>(current)
                    };
                }
                return Token{
                    TokenType::ASSIGN, "=", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case '!':
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::BANG_EQUAL, "!=", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                        static_cast<uint32_t>(start_pos),
                        static_cast<uint32_t>(current)
                    };
                }
                return Token{
                    TokenType::BANG, "!", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case '<':
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::LESS_EQUAL, "<=", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                        static_cast<uint32_t>(start_pos),
                        static_cast<uint32_t>(current)
                    };
                }
                if (peek() == '<') {
                    advance();
                    return Token{
                        TokenType::LESS_LESS, "<<", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                        static_cast<uint32_t>(start_pos),
                        static_cast<uint32_t>(current)
                    };
                }
                return Token{
                    TokenType::LESS, "<", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case '>':
                if (peek() == '=') {
                    advance();
                    return Token{
                        TokenType::GREATER_EQUAL, ">=", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                        static_cast<uint32_t>(start_pos),
                        static_cast<uint32_t>(current)
                    };
                }
                if (peek() == '>') {
                    advance();
                    return Token{
                        TokenType::GREATER_GREATER, ">>", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                        static_cast<uint32_t>(start_pos),
                        static_cast<uint32_t>(current)
                    };
                }
                return Token{
                    TokenType::GREATER, ">", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case '&':
                return Token{
                    TokenType::AMPERSAND, "&", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos), static_cast<uint32_t>(current)
                };
            case '|':
                return Token{
                    TokenType::PIPE, "|", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos), static_cast<uint32_t>(current)
                };
            case '^':
                return Token{
                    TokenType::CARET, "^", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos), static_cast<uint32_t>(current)
                };
            case '~':
                return Token{
                    TokenType::TILDE, "~", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos), static_cast<uint32_t>(current)
                };
            case '(': return Token{
                    TokenType::LEFT_PAREN, "(", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case ')': return Token{
                    TokenType::RIGHT_PAREN, ")", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case '{': return Token{
                    TokenType::LEFT_BRACE, "{", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case '}': return Token{
                    TokenType::RIGHT_BRACE, "}", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case '[': return Token{
                    TokenType::LEFT_BRACKET, "[", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case ']': return Token{
                    TokenType::RIGHT_BRACKET, "]", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case ';': return Token{
                    TokenType::SEMICOLON, ";", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case ',': return Token{
                    TokenType::COMMA, ",", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case '.': return Token{
                    TokenType::DOT, ".", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            case ':': return Token{
                    TokenType::COLON, ":", static_cast<uint16_t>(line), static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos),
                    static_cast<uint32_t>(current)
                };
            default:
                return Token{
                    TokenType::UNKNOWN, std::string_view(&source[start_pos], 1), static_cast<uint16_t>(line),
                    static_cast<uint16_t>(start_col),
                    static_cast<uint32_t>(start_pos), static_cast<uint32_t>(current)
                };
        }
    }
} // namespace ObSL
