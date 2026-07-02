#include <iostream>
#include <memory>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>

#include "Scripting/ObSLCore/Lexer/Lexer.h"
#include "Scripting/ObSLCore/Parser/Parser.h"
#include "Scripting/ObSLCore/ScriptRuntime.h"
#include "Core/ThreadPool.h"

using namespace ObSL;

static std::string read_file(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// TODO: probably merge runtime and scriptentry ?
static void test_single_worker(const std::vector<std::unique_ptr<Stmt> > &ast) {
    std::cout << "\n    Test Single worker (baseline)    \n";

    ScriptRuntime runtime;
    runtime.init(1);

    std::stringstream out;
    runtime.set_stdout(out);

    auto *worker = runtime.get_worker(0);
    auto env = worker->copy_globals();
    worker->GetInterpreter().register_environment(env);
    worker->execute(ast, env);

    std::cout << out.str();
    std::cout << "(single worker OK)\n";
}

static void test_concurrent_workers(const std::vector<std::unique_ptr<Stmt> > &ast) {
    constexpr size_t NUM_WORKERS = 4;
    std::cout << "\n    Test " << NUM_WORKERS << " concurrent workers    \n";

    ScriptRuntime runtime;
    runtime.init(NUM_WORKERS);

    std::vector<std::stringstream> outputs(NUM_WORKERS);
    for (size_t w = 0; w < NUM_WORKERS; ++w) {
        runtime.get_worker(w)->GetInterpreter().Set_Stdout(outputs[w]);
    }

    std::vector<std::shared_ptr<Environment> > envs(NUM_WORKERS);
    for (size_t w = 0; w < NUM_WORKERS; ++w) {
        auto *worker = runtime.get_worker(w);
        envs[w] = worker->copy_globals();
        worker->GetInterpreter().register_environment(envs[w]);
    }

    Core::ThreadPool pool(NUM_WORKERS);

    for (size_t w = 0; w < NUM_WORKERS; ++w) {
        pool.enqueue([&runtime, &ast, &envs, w]() {
            runtime.get_worker(w)->execute(ast, envs[w]);
        });
    }

    pool.wait();

    for (size_t w = 0; w < NUM_WORKERS; ++w) {
        std::cout << "    Worker " << w << " output    \n" << outputs[w].str();
    }

    std::cout << "(" << NUM_WORKERS << " concurrent workers OK)\n";
}


//          test independent environments
static void test_independent_environments() {
    constexpr size_t NUM_WORKERS = 4;
    std::cout << "\n    Test " << NUM_WORKERS << " workers, independent scripts    \n";

    const char *scripts[NUM_WORKERS] = {
        "var x = 1; var y = 2; assert(x + y == 3, \"w0\");",
        "var x = 10; var y = 20; assert(x + y == 30, \"w1\");",
        R"(var fruits = ["a","b","c"]; assert(fruits.len == 3, "w2");)",
        R"(fn fib(n) { if (n <= 1) return n; return fib(n-1) + fib(n-2); }
           assert(fib(10) == 55, "w3");)",
    };

    std::vector<std::vector<std::unique_ptr<Stmt> > > asts(NUM_WORKERS);
    for (size_t w = 0; w < NUM_WORKERS; ++w) {
        Lexer lexer(scripts[w]);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        asts[w] = parser.parse();
    }

    ScriptRuntime runtime;
    runtime.init(NUM_WORKERS);

    std::vector<std::stringstream> outputs(NUM_WORKERS);
    for (size_t w = 0; w < NUM_WORKERS; ++w) {
        runtime.get_worker(w)->GetInterpreter().Set_Stdout(outputs[w]);
    }

    std::vector<std::shared_ptr<Environment> > envs(NUM_WORKERS);
    for (size_t w = 0; w < NUM_WORKERS; ++w) {
        auto *worker = runtime.get_worker(w);
        envs[w] = worker->copy_globals();
        worker->GetInterpreter().register_environment(envs[w]);
    }

    Core::ThreadPool pool(NUM_WORKERS);
    for (size_t w = 0; w < NUM_WORKERS; ++w) {
        pool.enqueue([&runtime, &asts, &envs, w]() {
            runtime.get_worker(w)->execute(asts[w], envs[w]);
        });
    }
    pool.wait();

    for (size_t w = 0; w < NUM_WORKERS; ++w) {
        std::cout << "    Worker " << w << "    \n" << outputs[w].str();
    }
    std::cout << "(independent environments OK)\n";
}

int main() {
    try {
        const std::string script_content = read_file("test.obsl");

        // Parse ONCE
        // AST is readonly ;)
        Lexer lexer(script_content);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto statements = parser.parse();

        test_single_worker(statements);
        test_concurrent_workers(statements);
        test_independent_environments();

        std::cout << "\nAll thread safety tests passed!\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "\nFATAL: " << e.what() << "\n";
        return 1;
    }
}
