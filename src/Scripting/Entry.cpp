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
 * switch statements
 * foreach loop
 * proper in/out input buffers
 * limited native funcs & stl
 * engine intergration
 * lsp
 * docs lol
 */

/*
 todo:
 * TOKENS IN THE ENUM BUT NOT IMPLEMENTED
 * add token types to lexer:
 * switch
 * colons
 * case
 * default
 */


namespace ObSL {
    // this is just a test, move this to seperate file later and maybe make it more acessable...
    // p.s i really hate cpp lambda syntax
    void bind(Interpreter &m_interpreter) {
        m_interpreter.define_native("clock", 0, [](auto *, auto &) {
            const auto now = std::chrono::system_clock::now().time_since_epoch();
            return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count()) / 1000.0;
        });
    }


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
        bind(m_interpreter);
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
        bind(m_interpreter);
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
