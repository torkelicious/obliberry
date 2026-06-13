#ifndef OBLIBERRY_LEXER_H
#define OBLIBERRY_LEXER_H
#include <vector>
#include <string_view>
#include "Scripting/Tokens.h"

namespace ObSL {

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
