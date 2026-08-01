#include "Logger/Logger.h"


#include <filesystem>
#include <string>
#include "IO/Package/Tools/AssetPacking.h"
#include "IO/Package/Tools/DependencyGraph.h"
#include "IO/Package/Tools/IgnoreRules.h"
#include "IO/Package/Container.h"
#include "Logger/LoggerService.h"

const std::string TITLE_NAME = "Obliberry-Working-Name-Packager";
constexpr std::string BINARY_NAME = "ob_packer";
constexpr float VERSION = 1.1f;

namespace fs = std::filesystem;


#pragma push_macro("LOG_WHO")
#define LOG_WHO "Packer"

static void show_help() {
    std::cout << TITLE_NAME << " - Package Obliberry projects into .obpak archives\n\n"
              << "Usage: " << BINARY_NAME << " [options] <project_directory>\n\n"
              << "Options:\n"
              << "  -o, --output <file>    Output .obpak path (default: <project_directory>.obpak)\n"
              << "  -q, --quiet            Suppress all non-error output\n"
              << "  --verbose              Enable detailed logging per file\n"
              << "  --no-compress          Disable LZ4 compression globally\n"
              << "  --strict               Fail packaging on dependency validation errors\n"
              << "  -h, --help             Show this help message and exit\n"
              << "  -v, --version          Show version information and exit\n\n"
              << "Example:\n"
              << "  " << BINARY_NAME << " -o pkg/game.obpak ./UntitledProject\n";
}

static void show_version() { std::cout << BINARY_NAME << " (" << TITLE_NAME << ") v" << VERSION << "\n"; }

int main(int argc, char *argv[]) {
    Logging::Logger<256> logger;
    Logging::LoggerService::Initialize(&logger);

    if (argc < 2) {
        show_help();
        return 1;
    }

    std::string output_file;
    fs::path project_dir;
    bool quiet = false, verbose = false, global_compress = true, strict_mode = false;

    for (int i = 1; i < argc; ++i) {
        if (std::string arg = argv[i]; arg == "-o" || arg == "--output") {
            if (i + 1 < argc)
                output_file = argv[++i];
        } else if (arg == "-q" || arg == "--quiet")
            quiet = true;
        else if (arg == "--verbose")
            verbose = true;
        else if (arg == "--no-compress")
            global_compress = false;
        else if (arg == "--strict")
            strict_mode = true;
        else if (arg == "-h" || arg == "--help") {
            show_help();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            show_version();
            return 0;
        } else if (arg[0] == '-') {
            LOG_ERROR(LOG_WHO, "Unknown option: " + arg);
            show_help();
            return 1;
        } else
            project_dir = arg;
    }

    if (project_dir.empty()) {
        LOG_ERROR(LOG_WHO, "No project directory specified");
        return 1;
    }
    if (!fs::exists(project_dir) || !fs::is_directory(project_dir)) {
        LOG_ERROR(LOG_WHO, "Provided path is not a valid directory: " + project_dir.string());
        return 1;
    }
    if (output_file.empty())
        output_file = project_dir.filename().string() + ".obpak";

    if (!quiet) {
        LOG_INFO(LOG_WHO, "Packing project: " + project_dir.string());
        LOG_INFO(LOG_WHO, "Output file: " + output_file);
    }

    fs::path script_root = project_dir / "assets" / "scripts";
    if (!fs::exists(script_root)) {
        LOG_ERROR(LOG_WHO, "Expected scripts folder not found: " + script_root.string());
        return 1;
    }

    IO::ContainerWriter writer;
    IO::Package::Tools::DependencyGraph dep_graph;
    IO::Package::Tools::IgnoreRules ignore_rules = IO::Package::Tools::IgnoreRules::ForProject(project_dir);
    IO::Package::Tools::PackOptions opts{global_compress, verbose, quiet, BINARY_NAME};
    opts.ignore = &ignore_rules;

    int success_count = 0, fail_count = 0;

    for (fs::recursive_directory_iterator it(project_dir), end_it; it != end_it; ++it) {
        if (it->is_directory()) {
            // Prune ignored directories instead of descending into them.
            if (ignore_rules.IsIgnored(it->path(), /*is_dir=*/true))
                it.disable_recursion_pending();
            continue;
        }
        try {
            if (IO::Package::Tools::pack_one_file(it->path(), project_dir, script_root, writer, dep_graph, opts))
                ++success_count;
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, it->path().string() + " - " + e.what());
            ++fail_count;
        }
    }

    if (bool deps_ok = dep_graph.validate(BINARY_NAME); !deps_ok && strict_mode) {
        LOG_ERROR(LOG_WHO, "Packaging aborted due to validation failures (--strict)");
        return 1;
    }

    if (success_count > 0) {
        try {
            writer.write(output_file);
            if (!quiet) {
                LOG_INFO(LOG_WHO, "Wrote " + output_file);
                LOG_INFO(LOG_WHO, "Packed " + std::to_string(success_count) + "/" + std::to_string(success_count + fail_count) + " files.");
            }
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, std::string("Could not write package - ") + e.what());
            return 1;
        }
    } else {
        LOG_ERROR(LOG_WHO, "No files were successfully packed");
        return 1;
    }

    return fail_count == 0 ? 0 : 1;
}
#pragma pop_macro("LOG_WHO")
