#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

#include "IO/Package/Container.h"

const std::string TITLE_NAME = "Obliberry-Working-Name-Unpackager";
const std::string BINARY_NAME = "ob_unpack";
constexpr float VERSION = 1.1f;
namespace fs = std::filesystem;

static void show_help() {
    std::cout << TITLE_NAME << " - Extract contents from a .obpak container\n\n"
            << "Usage: " << BINARY_NAME << " [options] <package.obpak> [output_directory]\n\n"
            << "Options:\n"
            << "  -h, --help             Show this help message and exit\n"
            << "  -l, --list             List contents of obpack file without extracting\n"
            << "  -q, --quiet            Suppress output\n"
            << "  -v, --version          Show version\n\n"
            << "Example:\n"
            << "  " << BINARY_NAME << " game.obpak ./extracted_assets\n";
}

int main(int argc, char *argv[]) {
    bool list_contents = false;
    bool quiet = false;
    std::string package_path;
    std::string output_dir;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            show_help();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << TITLE_NAME << " v" << VERSION << "\n";
            return 0;
        } else if (arg == "-l" || arg == "--list") { list_contents = true; } else if (
            arg == "-q" || arg == "--quiet") { quiet = true; } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            show_help();
            return 1;
        } else {
            if (package_path.empty()) package_path = arg;
            else if (output_dir.empty()) output_dir = arg;
            else {
                std::cerr << "Unexpected extra argument: " << arg << "\n";
                show_help();
                return 1;
            }
        }
    }

    if (package_path.empty()) {
        show_help();
        return 1;
    }

    IO::ContainerReader reader;
    if (!reader.open(package_path)) {
        std::cerr << "Fatal Error: Could not open package '" << package_path << "'\n";
        return 1;
    }

    if (list_contents) {
        reader.print_entries();
        return 0;
    }

    // default to a folder named after the package if no output dir is provided
    if (output_dir.empty()) {
        output_dir = fs::path(package_path).stem().string();
    }

    auto paths = reader.get_entry_paths();
    if (!quiet) std::cout << "Extracting " << paths.size() << " files to " << output_dir << "...\n";

    int success_count = 0;
    for (const auto &p: paths) {
        auto data = reader.read(p);
        if (!data) {
            std::cerr << "  [Error] Failed to decompress: " << p << "\n";
            continue;
        }

        fs::path out_path = fs::path(output_dir) / p;
        fs::create_directories(out_path.parent_path());

        std::ofstream out(out_path, std::ios::binary);
        if (out.write(data->data(), data->size())) {
            if (!quiet) std::cout << "  Extracted: " << p << " (" << data->size() << " bytes)\n";
            success_count++;
        } else {
            std::cerr << "  [Error] Failed to write to disk: " << out_path << "\n";
        }
    }

    if (!quiet) std::cout << "\nFinished extracting " << success_count << "/" << paths.size() << " files.\n";
    return (success_count == paths.size()) ? 0 : 1;
}
