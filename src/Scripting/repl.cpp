#include "repl.h"
#include "Lexer/Lexer.h"
#include <iostream>
#include <memory>
#include <string>

namespace Scripting {
    void print_tokens(const std::vector<Token> &tokens, Lexer &lex) {
        for (const auto &[type, lexeme, line, column, start_pos, end_pos]: tokens) {
            std::cout << "Token{"
                    << "Type: " << static_cast<int>(type)
                    << " | Lexeme: '" << lexeme << "'"
                    << " | Line: " << line
                    << ", Col: " << column
                    << " | Span: [" << start_pos << " -> " << end_pos << "]}\n";
        }
    }

    void start_repl() {
        std::string line;
        while (true) {
            std::cout << "> ";
            if (!std::getline(std::cin, line)) break;

            if (line == "exit") {
                std::cout << "Goodbye!\n";
                break;
            }
            Lexer lexer(line);
            std::vector<Token> tokens = lexer.tokenize();
            print_tokens(tokens, lexer);
        }
    }
}
