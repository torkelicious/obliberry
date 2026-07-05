#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include <ObSL/ScriptRuntime.h>
#include <ObSL/ScriptWorker.h>
#include <ObSL/Environment.h>
#include <ObSL/Interpreter.h>

#include "ASTDeserializer.h"
#include "IO/Package/Container.h"

void print_usage(const char *prog_name) {
    std::cout << "ObSL Packaged Runner\n"
            << "Usage: " << prog_name << " <package.obpak> <entry_script_path>\n\n"
            << "Example:\n"
            << "  " << prog_name << " assets.obpak scripts/main.obsl\n";
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    std::string package_path = argv[1];
    std::string entry_script = argv[2];

    IO::ContainerReader reader;
    if (!reader.open(package_path)) {
        std::cerr << "Fatal Error: Could not open package '" << package_path << "'\n";
        return 1;
    }

    auto raw_data_opt = reader.read(entry_script);
    if (!raw_data_opt) {
        std::cerr << "Fatal Error: Could not find '" << entry_script << "' in the package.\n";
        return 1;
    }

    const std::string &raw_data = *raw_data_opt;
    std::vector<uint8_t> binary_blob(raw_data.begin(), raw_data.end());

    try {
        std::cout << "Deserializing " << binary_blob.size() << " bytes for " << entry_script << "...\n";
        auto [string_pool, statements] = ObSL::ASTDeserializer::deserialize(binary_blob);

        ObSL::ScriptRuntime runtime;
        runtime.init(".", 1);

        ObSL::ScriptWorker *worker = runtime.get_worker(0);
        auto globals = worker->copy_globals();

        // Execute the AST Directly
        std::cout << "Executing...\n-----------------------------------\n";

        worker->execute(statements, globals);

        std::cout << "\n-----------------------------------\nExecution finished successfully.\n";
    } catch (const std::exception &e) {
        std::cerr << "\n[Runtime Error]: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
