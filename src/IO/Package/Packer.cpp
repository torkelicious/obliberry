#include <iostream>
#include <filesystem>
#include <string>

#include "IO/Package/Tools/CliCommon.h"
#include "IO/Package/Tools/AssetPacking.h"
#include "IO/Package/Tools/DependencyGraph.h"
#include "IO/Package/Container.h"

const std::string TITLE_NAME = "Obliberry-Working-Name-Packager";
const std::string BINARY_NAME = "ob_packer";
constexpr float VERSION = 1.1f;

namespace fs = std::filesystem;

static void log_info(const std::string &msg) { IO::Package::Tools::log_info(BINARY_NAME, msg); }
static void log_error(const std::string &msg) { IO::Package::Tools::log_error(BINARY_NAME, msg); }

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

static void show_version() {
    std::cout << BINARY_NAME << " (" << TITLE_NAME << ") v" << VERSION << "\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        show_help();
        return 1;
    }

    std::string output_file;
    fs::path project_dir;
    bool quiet = false, verbose = false, global_compress = true, strict_mode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" || arg == "--output") { if (i + 1 < argc) output_file = argv[++i]; } else if (
            arg == "-q" || arg == "--quiet") quiet = true;
        else if (arg == "--verbose") verbose = true;
        else if (arg == "--no-compress") global_compress = false;
        else if (arg == "--strict") strict_mode = true;
        else if (arg == "-h" || arg == "--help") {
            show_help();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            show_version();
            return 0;
        } else if (arg[0] == '-') {
            log_error("Unknown option: " + arg);
            show_help();
            return 1;
        } else project_dir = arg;
    }

    if (project_dir.empty()) {
        log_error("No project directory specified.");
        return 1;
    }
    if (!fs::exists(project_dir) || !fs::is_directory(project_dir)) {
        log_error("Provided path is not a valid directory: " + project_dir.string());
        return 1;
    }
    if (output_file.empty()) output_file = project_dir.filename().string() + ".obpak";

    if (!quiet) {
        log_info("Packing project: " + project_dir.string());
        log_info("Output file: " + output_file);
    }

    IO::ContainerWriter writer;
    IO::Package::Tools::DependencyGraph dep_graph;
    IO::Package::Tools::PackOptions opts{global_compress, verbose, quiet, BINARY_NAME};

    int success_count = 0, fail_count = 0;

    for (const auto &entry: fs::recursive_directory_iterator(project_dir)) {
        if (entry.is_directory()) continue;
        try {
            if (IO::Package::Tools::pack_one_file(entry.path(), project_dir, writer, dep_graph, opts))
                ++success_count;
        } catch (const std::exception &e) {
            log_error(entry.path().string() + " - " + e.what());
            ++fail_count;
        }
    }

    bool deps_ok = dep_graph.validate(BINARY_NAME);
    if (!deps_ok && strict_mode) {
        log_error("Packaging aborted due to validation failures (--strict).");
        return 1;
    }

    if (success_count > 0) {
        try {
            writer.write(output_file);
            if (!quiet) {
                log_info("Wrote " + output_file);
                log_info("Packed " + std::to_string(success_count) + "/" +
                         std::to_string(success_count + fail_count) + " files.");
            }
        } catch (const std::exception &e) {
            log_error(std::string("Could not write package - ") + e.what());
            return 1;
        }
    } else {
        log_error("No files were successfully packed.");
        return 1;
    }

    return fail_count == 0 ? 0 : 1;
}
