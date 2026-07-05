#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <ObSL/ScriptRuntime.h>
#include <ObSL/ScriptWorker.h>
#include "../../Scripting/ASTPackager/ASTDeserializer.h"
#include "IO/Package/Container.h"

const std::string TITLE_NAME = "Obliberry-Working-Name-Runner";
const std::string BINARY_NAME = "obsl_pack_run";
constexpr float VERSION = 1.0f;

static void show_help() {
    std::cout << TITLE_NAME << " - Run pre-packaged ObSL scripts directly from a .obpak container\n\n"
            << "Usage: " << BINARY_NAME << " [options] <package.obpak> <entry_script_path>\n\n"
            << "Options:\n"
            << "  -h, --help             Show this help message and exit\n"
            << "  -v, --version          Show version information and exit\n"
            << "  -q, --quiet            Suppress runner output (script print output is preserved)\n\n"
            << "Example:\n"
            << "  " << BINARY_NAME << " -q game.obpak assets/scripts/engine_integ_test.obsl\n";
}

int main(int argc, char *argv[]) {
    bool quiet = false;
    std::string package_path;
    std::string entry_script;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            show_help();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << TITLE_NAME << " v" << VERSION << "\n";
            return 0;
        } else if (arg == "-q" || arg == "--quiet") {
            quiet = true;
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            show_help();
            return 1;
        } else {
            if (package_path.empty()) {
                package_path = arg;
            } else if (entry_script.empty()) {
                entry_script = arg;
            } else {
                std::cerr << "Unexpected extra argument: " << arg << "\n";
                show_help();
                return 1;
            }
        }
    }

    if (package_path.empty() || entry_script.empty()) {
        show_help();
        return 1;
    }

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
        if (!quiet) {
            std::cout << "[ob_run] Deserializing " << binary_blob.size() << " bytes...\n";
        }

        auto [string_pool, statements] = ObSL::ASTDeserializer::deserialize(binary_blob);

        ObSL::ScriptRuntime runtime;
        runtime.init(".", 1);

        ObSL::ScriptWorker *worker = runtime.get_worker(0);
        auto globals = worker->copy_globals();

        if (!quiet) {
            std::cout << "[ob_run] Executing...\n-----------------------------------\n";
        }

        worker->execute(statements, globals);

        if (!quiet) {
            std::cout << "\n-----------------------------------\n[ob_run] Done.\n";
        }
    } catch (const std::exception &e) {
        std::cerr << "\n[Runtime Error]: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
