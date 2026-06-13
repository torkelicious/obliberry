#ifndef OBLIBERRY_TOKENS_H
#define OBLIBERRY_TOKENS_H
#include <string_view>

namespace ObSL {
    enum class TokenType {
        // Punctuation & Single character tokens
        LEFT_PAREN,
        RIGHT_PAREN,
        LEFT_BRACE,
        RIGHT_BRACE,
        COMMA,
        DOT,
        SEMICOLON,

        // Operators
        OPERATOR,
        MINUS,
        PLUS,
        STAR,
        SLASH,
        BANG,
        BANG_EQUAL,
        EQUAL_EQUAL,
        ASSIGN,
        GREATER,
        GREATER_EQUAL,
        LESS,
        LESS_EQUAL,

        // Literals & Identifiers
        STRING,
        NUMBER,
        IDENTIFIER,

        // Keywords
        AND,
        ELSE,
        FALSE_,
        FOR,
        FN, // short for function if this is confusing 4 anyone... (i.e my forgetfull self)
        IF,
        NULL_,
        OR,
        PRINT,
        RETURN,
        BREAK,
        THIS,
        TRUE_,
        VAR,
        WHILE,

        // Special
        EOF_,
        UNKNOWN
    };

    struct Token {
        TokenType type;
        std::string_view lexeme;
        int line;
        int column;
        int start_pos;
        int end_pos;
    };
}

#endif //OBLIBERRY_TOKENS_H
