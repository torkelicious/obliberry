#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "IO/Package/Tools/CliCommon.h"
#include "IO/Package/Container.h"

const std::string TITLE_NAME = "Obliberry-Working-Name-Unpackager";
const std::string BINARY_NAME = "ob_unpack";
constexpr float VERSION = 1.1f;
namespace fs = std::filesystem;

static void log_info(const std::string &msg) { IO::Package::Tools::log_info(BINARY_NAME, msg); }
static void log_error(const std::string &msg) { IO::Package::Tools::log_error(BINARY_NAME, msg); }

static void show_help() {
    std::cout << TITLE_NAME << " - Extract contents from a .obpak container\n\n"
              << "Usage: " << BINARY_NAME << " [options] <package.obpak> [output_directory]\n\n"
              << "Options:\n"
              << "  -h, --help             Show this help message and exit\n"
              << "  -l, --list             List contents of obpak file without extracting\n"
              << "  -q, --quiet            Suppress output\n"
              << "  -v, --version          Show version\n\n"
              << "Example:\n"
              << "  " << BINARY_NAME << " game.obpak ./extracted_assets\n";
}

int main(int argc, char *argv[]) {
    bool list_contents = false;
    bool quiet = false;
    bool readable = false;
    // only applies to json since i cant be arsed to write a fucking binary ast & de-parser and back to lexemes, well i
    // could, but i dont want too.
    std::string package_path;
    std::string output_dir;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            show_help();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << BINARY_NAME << " (" << TITLE_NAME << ") v" << VERSION << "\n";
            return 0;
        } else if (arg == "-l" || arg == "--list") {
            list_contents = true;
        } else if (arg == "-q" || arg == "--quiet") {
            quiet = true;
        } else if (arg == "-r" || arg == "--readable") {
            readable = true;
        } else if (arg[0] == '-') {
            log_error("Unknown option: " + arg);
            show_help();
            return 1;
        } else {
            if (package_path.empty())
                package_path = arg;
            else if (output_dir.empty())
                output_dir = arg;
            else {
                log_error("Unexpected extra argument: " + arg);
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
        log_error("Could not open package '" + package_path + "'");
        return 1;
    }

    if (list_contents) {
        reader.print_entries();
        return 0;
    }

    if (output_dir.empty()) {
        output_dir = fs::path(package_path).stem().string();
    }

    auto paths = reader.get_entry_paths();
    if (!quiet)
        log_info("Extracting " + std::to_string(paths.size()) + " files to " + output_dir + "...");

    int success_count = 0;
    for (const auto &p : paths) {
        auto data = reader.read(p);
        if (!data) {
            log_error("Failed to decompress: " + p);
            continue;
        }

        fs::path out_path = fs::path(output_dir) / p;
        fs::create_directories(out_path.parent_path());

        if (fs::path(p).extension() == ".json" && readable) {
            try {
                std::vector<uint8_t> bytes(data->begin(), data->end());
                nlohmann::json j = nlohmann::json::from_msgpack(bytes);
                std::string pretty = j.dump(2);
                std::ofstream out(out_path, std::ios::binary);
                out << pretty;
                if (!quiet)
                    log_info("Extracted (json): " + p + " (" + std::to_string(pretty.size()) + " bytes)");
                success_count++;
                continue;
            } catch (const std::exception &e) {
                log_error("Failed to decode msgpack for " + p + ": " + e.what());
                // falls through to standard binary
            }
        }

        std::ofstream out(out_path, std::ios::binary);
        if (out.write(data->data(), data->size())) {
            if (!quiet)
                log_info("Extracted: " + p + " (" + std::to_string(data->size()) + " bytes)");
            success_count++;
        } else {
            log_error("Failed to write to disk: " + out_path.string());
        }
    }


    if (!quiet)
        log_info("Finished extracting " + std::to_string(success_count) + "/" + std::to_string(paths.size()) +
                 " files.");
    return (static_cast<size_t>(success_count) == paths.size()) ? 0 : 1;
}
