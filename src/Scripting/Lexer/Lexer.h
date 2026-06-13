#ifndef OBLIBERRY_LEXER_H
#define OBLIBERRY_LEXER_H
#include <vector>
#include <string_view>

namespace Scripting {
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
        FUN,
        IF,
        NULL_,
        OR,
        PRINT,
        RETURN,
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

    class Lexer {
    public:
        explicit Lexer(std::string_view source);

        std::vector<Token> tokenize();

    private:
        std::string_view source;
        size_t current = 0;
        int line = 1;
        int column = 1;

        [[nodiscard]] char peek() const;

        [[nodiscard]] char peek_next() const;

        char advance();

        [[nodiscard]] bool is_at_end() const;

        void skip_whitespace();

        Token read_string();

        Token read_num();

        Token read_identifier_or_keyword();

        Token read_operator_or_symbol();
    };
}

#endif //OBLIBERRY_LEXER_H
