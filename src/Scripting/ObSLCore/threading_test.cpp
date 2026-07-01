#include <iostream>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"
#include "Scripting/ObSLCore/Parser/Parser.h"
#include "Scripting/ObSLCore/Lexer/Lexer.h"

using namespace ObSL;

std::string read_file(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void test_thread_safety() {
    std::cout << "\n(ObSL) Testing Thread Safety\n";

    try {
        // Read the test script
        const std::string script_content = read_file("test.obsl");

        // Create a single interpreter instance
        Interpreter interpreter;

        // Parse the script once
        Lexer lexer(script_content);
        const auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto statements = parser.parse();

        std::cout << "\nRunning test script in single thread first...\n";

        // Run in single thread first to establish baseline
        interpreter.interpret(statements);

        std::cout << "\nRunning test script in multiple threads...\n";

        // Create multiple threads that will use the same interpreter
        std::vector<std::thread> threads;
        const int num_threads = 4;

        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back([&interpreter, &statements, i]() {
                try {
                    std::cout << "\nThread " << i << " starting...\n";
                    interpreter.interpret(statements);
                    std::cout << "\nThread " << i << " completed successfully.\n";
                } catch (const std::exception &e) {
                    std::cerr << "\nThread " << i << " failed: " << e.what() << "\n";
                }
            });
        }

        // Wait for all threads to complete
        for (auto &thread: threads) {
            thread.join();
        }

        std::cout << "\nTest Completed\n";
    } catch (const std::exception &e) {
        std::cerr << "\nTest failed: " << e.what() << "\n";
        return;
    }
}

int main() {
    test_thread_safety();
    return 0;
}
