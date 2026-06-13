#include "Lexer.h"
#include <cctype>

namespace Scripting {
    Lexer::Lexer(std::string_view source)
        : source(source) {
    }

    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> tokens;
        while (!is_at_end()) {
            skip_whitespace();

            char c = peek();

            if (isdigit(c)) {
                tokens.push_back(read_num());
            } else if (isalpha(c)) {
                tokens.push_back(read_identifier_or_keyword());
            } else {
                tokens.push_back(read_operator_or_symbol());
            }
        }
        tokens.push_back(Token(TokenType::EOF_, "", line, column));
        return tokens;
    }

    char Lexer::peek() const {
        return is_at_end() ? '\0' : source[current];
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
        while (!is_at_end()) {
            char chr = peek();
            if (chr == ' ' || chr == '\t') {
                advance();
            } else if (chr == '\n') {
                advance();
                ++line;
                column = 1;
            } else {
                break;
            }
        }
    }

    Token Lexer::read_num() {
        size_t number_start = current;
        while (isdigit(peek())) { advance(); }
        auto lexeme = source.substr(number_start, current - number_start);
        return Token(TokenType::NUMBER, lexeme, line, column);
    }

    Token Lexer::read_identifier_or_keyword() {
        size_t id_start = current;
        while (isalnum(peek()) || peek() == '_') { advance(); }
        auto text = source.substr(id_start, current - id_start);
        if (text == "print") return Token(TokenType::PRINT, text, line, column);
        if (text == "if") return Token(TokenType::IF, text, line, column);
        if (text == "while") return Token(TokenType::WHILE, text, line, column);
        return Token(TokenType::IDENTIFIER, text, line, column);
    }

    Token Lexer::read_operator_or_symbol() {
        char c = advance();
        switch (c) {
            case '+':
                return Token(TokenType::PLUS, source.substr(current - 1, 1), line, column);
            case '-':
                return Token(TokenType::MINUS, source.substr(current - 1, 1), line, column);
            case '*':
                return Token(TokenType::STAR, source.substr(current - 1, 1), line, column);
            case '/':
                return Token(TokenType::SLASH, source.substr(current - 1, 1), line, column);
            // - - -
            case '=':
                return Token(TokenType::ASSIGN, source.substr(current - 1, 1), line, column);
            case '(': return Token(TokenType::LEFT_PAREN, "(", line, column);

            case ')': return Token(TokenType::RIGHT_PAREN, ")", line, column);

            case '{': return Token(TokenType::LEFT_BRACE, "{", line, column);
            case '}': return Token(TokenType::RIGHT_BRACE, "}", line, column);
            case ';': return Token(TokenType::SEMICOLON, ";", line, column);
            default:
                return Token(TokenType::UNKNOWN, source.substr(current - 1, 1), line, column);
        }
    }

const char* Lexer::stringify(Scripting::TokenType type) {
    switch (type) {
        case Scripting::TokenType::LEFT_PAREN: return "LEFT_PAREN";
        case Scripting::TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
        case Scripting::TokenType::LEFT_BRACE: return "LEFT_BRACE";
        case Scripting::TokenType::RIGHT_BRACE: return "RIGHT_BRACE";
        case Scripting::TokenType::COMMA: return "COMMA";
        case Scripting::TokenType::DOT: return "DOT";
        case Scripting::TokenType::SEMICOLON: return "SEMICOLON";
        case Scripting::TokenType::MINUS: return "MINUS";
        case Scripting::TokenType::PLUS: return "PLUS";
        case Scripting::TokenType::STAR: return "STAR";
        case Scripting::TokenType::SLASH: return "SLASH";
        case Scripting::TokenType::BANG: return "BANG";
        case Scripting::TokenType::BANG_EQUAL: return "BANG_EQUAL";
        case Scripting::TokenType::EQUAL_EQUAL: return "EQUAL_EQUAL";
        case Scripting::TokenType::ASSIGN: return "ASSIGN";
        case Scripting::TokenType::GREATER: return "GREATER";
        case Scripting::TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case Scripting::TokenType::LESS: return "LESS";
        case Scripting::TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case Scripting::TokenType::STRING: return "STRING";
        case Scripting::TokenType::NUMBER: return "NUMBER";
        case Scripting::TokenType::IDENTIFIER: return "IDENTIFIER";
        case Scripting::TokenType::AND: return "AND";
        case Scripting::TokenType::ELSE: return "ELSE";
        case Scripting::TokenType::FALSE_: return "FALSE_";
        case Scripting::TokenType::FOR: return "FOR";
        case Scripting::TokenType::FUN: return "FUN";
        case Scripting::TokenType::IF: return "IF";
        case Scripting::TokenType::NULL_: return "NULL_";
        case Scripting::TokenType::OR: return "OR";
        case Scripting::TokenType::PRINT: return "PRINT";
        case Scripting::TokenType::RETURN: return "RETURN";
        case Scripting::TokenType::THIS: return "THIS";
        case Scripting::TokenType::TRUE_: return "TRUE_";
        case Scripting::TokenType::VAR: return "VAR";
        case Scripting::TokenType::WHILE: return "WHILE";
        case Scripting::TokenType::EOF_: return "EOF_";
        case Scripting::TokenType::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}


}
