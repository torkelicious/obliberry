#ifndef OBLIBERRY_LEXER_H
#define OBLIBERRY_LEXER_H
#include <string>
#include <variant>
#include <vector>

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
        // todo: add ecs and game tokens once basic interpreting is done!!!
    };


    struct Token {
        TokenType type;
        std::string_view lexeme;
        int line; // maybe use uint32 or size_t later on if i dont need to do weird maths with this
        int column;
        int start_pos;
        int end_pos;
    };

    class Lexer {
    public:
        Lexer(std::string_view source);

        std::vector<Token> tokenize();

    private:
        std::string_view source;
        //size_t start = 0; // unused for now but will pop up later
        size_t current = 0;
        int line = 1;
        int column = 1;

        char peek() const;

        char peek_next() const;

        char advance();

        bool is_at_end() const;

        void skip_whitespace();

        Token read_string();

        Token read_num();

        Token read_identifier_or_keyword();

        Token read_operator_or_symbol();
    };
}

#endif //OBLIBERRY_LEXER_H
