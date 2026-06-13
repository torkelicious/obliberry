#include "repl.h"
#include "Lexer/Lexer.h"
#include "Parser/Parser.h"
#include <iostream>
#include <string>
#include <vector>

namespace ObSL {
    void start_repl() {
        std::string line;
        std::cout << "obsl REPL\nType 'exit' to quit.\n";
        while (true) {
            std::cout << "> ";
            if (!std::getline(std::cin, line)) break;
            if (line == "exit") {
                std::cout << "Goodbye!\n";
                break;
            }
            Lexer lexer(line);
            const std::vector<Token> tokens = lexer.tokenize();
            Parser parser(tokens);
            try {
                // ast nodes
                for (auto statements = parser.parse(); const auto &stmt: statements) {
                    if (stmt) {
                        std::cout << stmt->to_string();
                    }
                }
            } catch (const std::runtime_error &e) {
                std::cerr << "Parser Error: " << e.what() << "\n";
            }
        }
    }
}
