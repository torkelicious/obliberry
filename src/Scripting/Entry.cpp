#include "Entry.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <json.hpp>
#include "Scripting/Lexer/Lexer.h"
#include "Scripting/Parser/Parser.h"

/*
 * ObSL todo:
 * engine intergration
 * lsp
 * docs lol
 */

namespace ObSL {
    int Entry::exec(const int argc, char *argv[]) {
        std::setvbuf(stdin, nullptr, _IONBF, 0);
        std::setvbuf(stdout, nullptr, _IONBF, 0);

        bool lint_mode = false;
        std::string file_path = "";

        for (int i = 1; i < argc; ++i) {
            if (std::string arg = argv[i]; arg == "--lint") {
                lint_mode = true;
            } else if (file_path.empty()) {
                file_path = arg;
            } else {
                std::cout << "Usage: obsl [--lint] [script.obsl]\n";
                return 64;
            }
        }
        if (lint_mode) {
            if (file_path.empty()) {
                nlohmann::json output;
                output["status"] = "error";
                output["errors"].push_back({
                    {"line", 1},
                    {"column", 1},
                    {"message", "No input file specified for linting."}
                });
                std::cout << output.dump() << std::endl;
                return 64;
            }
            run_lint(file_path);
            return 0;
        }


        if (!file_path.empty()) {
            runFile(file_path);
        } else {
            runREPL();
        }
        return 0;
    }

    void Entry::runFile(const std::string &path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file '" << path << "'\n";
            std::exit(74);
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
            const auto statements = parser.parse();
            m_interpreter.interpret(statements);
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << "\n";
        } catch (...) {
            std::cerr << "ObSL Error: An unknown exception occurred.\n";
        }
    }

    void Entry::run_lint(const std::string &path) {
        // base JSON response object
        nlohmann::json output;
        output["errors"] = nlohmann::json::array();

        std::ifstream file(path);
        if (!file.is_open()) {
            output["status"] = "error";
            output["errors"].push_back({
                {"line", 1},
                {"column", 1},
                {"message", "Could not open file: " + path}
            });
            std::cout << output.dump() << std::endl;
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        try {
            Lexer lexer(source);
            const auto tokens = lexer.tokenize();

            Parser parser(tokens);
            const auto statements = parser.parse();

            // the AST was built successfully
            output["status"] = "ok";
        } catch (const RuntimeError &e) {
            output["status"] = "error";

            size_t report_line = e.token.line;
            size_t report_col = e.token.column;

            // If the parser hits a closing brace or EOF on a brand new line,
            // it means the missing semicolon belongs at the end of the line b4 it.

            // todo: fix line messing up on braces/eof this is a hack i guess. (maybe implement wrapper idk?)
            if ((e.token.type == TokenType::RIGHT_BRACE || e.token.type == TokenType::EOF_)
                && report_line > 1) {
                report_line -= 1;
                report_col = 1;
            }
            output["errors"].push_back({
                {"line", report_line},
                {"column", report_col},
                {"message", e.what()}
            });
        }

        // dump the JSON to stdout
        std::cout << output.dump() << std::endl;
    }
}
