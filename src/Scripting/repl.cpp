#include "repl.h"
#include "Lexer/Lexer.h"
#include <iostream>
#include <memory>
#include <string>

namespace Scripting {
    void print_tokens(const std::vector<Token> &tokens, Lexer &lex) {
        for (const auto &token: tokens) {
            std::cout << "Token{" << lex.stringify(token.type)
                    << ", " << token.lexeme
                    << ", line " << token.line
                    << ", column " << token.column << "}\n";
        }
    }

    void start_repl() {
        std::string line;
        while (std::cout << "> " && std::getline(std::cin, line)) {
            if (line == "exit") {
                std::cout << "Goodbye!\n";
                break;
            }
            Lexer lexer(line);
            std::vector<Scripting::Token> tokens = lexer.tokenize();
            print_tokens(tokens, lexer);
        }
    }
}
