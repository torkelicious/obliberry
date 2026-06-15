#include "Entry.h"

#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include "Scripting/Lexer/Lexer.h"
#include "Scripting/Parser/Parser.h"

/*
 * ObSL todo:
 * bitwise operators
 * regex & networking?? (maybe)
 * better error handling (try catch ?)
 * structs
 * function default args
 * reflection
 * engine intergration
 * lsp
 * docs lol
 */

namespace ObSL {
    int Entry::exec(const int argc, char *argv[]) {
        if (argc > 2) {
            std::cout << "takes arg [script.obsl]\n";
            return 64;
        }
        if (argc == 2) {
            runFile(argv[1]);
        } else {
            runREPL();
        }
        return 0;
    }

    void Entry::runFile(const std::string &path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file '" << path << "'\n";
            std::exit(74); // io err
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        run(buffer.str());
    }

    void Entry::runREPL() {
        std::string line;
        std::cout << "ObSL REPL (type 'exit' to quit)\n> ";
        while (std::getline(std::cin, line)) {
            if (line == "exit") break;
            if (!line.empty()) {
                run(line);
            }
            std::cout << "\n> ";
        }
    }

    void Entry::run(const std::string &source) {
        try {
            Lexer lexer(source);
            const auto tokens = lexer.tokenize();
            Parser parser(tokens);
            auto statements = parser.parse();
            m_interpreter.interpret(std::move(statements));
        } catch (const std::exception &e) {
            std::cerr << "ObSL Error: " << e.what() << "\n";
        } catch (...) {
            std::cerr << "ObSL Error: An unknown exception occurred.\n";
        }
    }
}
