#include "repl.h"
#include "Lexer/Lexer.h"
#include "Parser/Parser.h"
#include <iostream>
#include <string>
#include <vector>

#include "Interpreter/Interpreter.h"

/*
 *todo:
 * switch statements
 * functions & closures
 * data types like list/arrays
 * foreach
 * some (limited) native funcs? stl? (later)
 * engine intergration (later later!!!)
 * execute from file...
 */

/* todo:
 * TOKENS IN THE ENUM BUT NOT IMPLEMENTED
 * add token types to lexer:
 * l/r brackets
 * switch
 * colons
 * case
 * default
 */


namespace ObSL {
    void start_repl() {
        std::string line;
        std::cout << "ObSL REPL\nType 'exit' to quit.\n";
        // global state is held in sesh
        Interpreter interpreter;
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
                auto statements = parser.parse();
                // pass the generated AST into the executer
                interpreter.interpret(statements);
            } catch (const std::runtime_error &e) {
                // parser syntax errors or interpreter runtime errors
                std::cerr << "Error: " << e.what() << "\n";
            }
        }
    }
}
