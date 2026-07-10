#include <string>
#include <vector>
#include <filesystem>
#include <ObSL/ScriptRuntime.h>
#include <ObSL/ScriptWorker.h>
#include <ObSL/ASTDeserializer.h>
#include "IO/Package/Container.h"
#include "Core/LoggerService.h"

const std::string TITLE_NAME = "Obliberry-Working-Name-Runner";
const std::string BINARY_NAME = "obsl_pack_run";
constexpr float VERSION = 1.0f;

constexpr auto LOG_WHO = "Runner";

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
            std::cout << BINARY_NAME << " (" << TITLE_NAME << ") v" << VERSION << "\n";
            return 0;
        } else if (arg == "-q" || arg == "--quiet") {
            quiet = true;
        } else if (arg[0] == '-') {
            LOG_ERROR(LOG_WHO, "Unknown option: " + arg);
            show_help();
            return 1;
        } else {
            if (package_path.empty()) {
                package_path = arg;
            } else if (entry_script.empty()) {
                entry_script = arg;
            } else {
                LOG_ERROR(LOG_WHO, "Unexpected extra argument: " + arg);
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
        LOG_ERROR(LOG_WHO, "Could not open package '" + package_path + "'");
        return 1;
    }

    auto raw_data_opt = reader.read(entry_script);
    if (!raw_data_opt) {
        LOG_ERROR(LOG_WHO, "Could not find '" + entry_script + "' in the package");
        return 1;
    }

    const std::string &raw_data = *raw_data_opt;
    std::vector<uint8_t> binary_blob(raw_data.begin(), raw_data.end());

    try {
        if (!quiet)
            log_info("Deserializing " + std::to_string(binary_blob.size()) + " bytes...");

        auto [string_pool, statements] = ObSL::ASTDeserializer::deserialize(binary_blob);

        ObSL::ScriptRuntime runtime;
        runtime.init(".", 1);

        ObSL::ScriptWorker *worker = runtime.get_worker(0);
        auto globals = worker->copy_globals();

        // Install a module loader that reads precompiled ASTs from the package
        worker->GetInterpreter().set_module_loader(
                [&reader](const std::string &path) -> std::optional<ObSL::ModuleResult> {
                    auto data = reader.read(path);
                    if (!data)
                        return std::nullopt;

                    const std::vector<uint8_t> binary_blob(data->begin(), data->end());
                    ObSL::ModuleResult result;
                    result.kind = ObSL::ModuleResult::Kind::PrecompiledAst;
                    result.ast_module = ObSL::ASTDeserializer::deserialize(binary_blob);
                    return result;
                });


        if (!quiet)
            log_info("Executing...\n-----------------------------------");

        worker->execute(statements, globals);

        if (!quiet)
            std::cout << "\n-----------------------------------\n";
        if (!quiet)
            log_info("Done.");
    } catch (const std::exception &e) {
        LOG_ERROR(LOG_WHO, std::string("Runtime error: ") + e.what());
        return 1;
    }
    return 0;
}
