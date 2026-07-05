#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>

#include <ObSL/Lexer.h>
#include <ObSL/Parser.h>
#include <ObSL/ScriptEntry.h>
#include <nlohmann/json.hpp>

#include "../../Scripting/ASTPackager/ASTSerializer.h"
#include "IO/Package/Container.h"


const std::string TITLE_NAME = "Obliberry-Working-Name-Packager";
const std::string BINARY_NAME = "ob_packer";
constexpr float VERSION = 1.1f;

namespace fs = std::filesystem;

// Helpers
static std::vector<uint8_t> read_file_binary(const fs::path &filepath) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filepath.string());
    }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

static std::string read_file_string(const fs::path &filepath) {
    auto data = read_file_binary(filepath);
    return std::string(data.begin(), data.end());
}

static void show_help() {
    std::cout <<
            TITLE_NAME <<
            " - Package Obliberry projects into .obpak archives\n"
            "\n"
            "Usage: " << BINARY_NAME << " [options] <project_directory>\n"
            "\n"
            "Options:\n"
            "  -o, --output <file>    Output .obpak path\n"
            "                         (default: <project_directory>.obpak)\n"
            "  -q, --quiet            Suppress all non-error output\n"
            "  --verbose              Enable detailed logging per file\n"
            "  --no-compress          Disable LZ4 compression globally\n"
            "  -h, --help             Show this help message and exit\n"
            "  -v, --version          Show version information and exit\n"
            "\n"
            "Example:\n"
            "  " << BINARY_NAME << " -o pkg/game.obpak ./UntitledProject\n";
}

static void show_version() {
    std::cout << TITLE_NAME << " v" << VERSION << "\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        show_help();
        return 1;
    }

    std::string output_file;
    fs::path project_dir;
    bool quiet = false;
    bool verbose = false;
    bool global_compress = true;

    // Parse Args
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) output_file = argv[++i];
        } else if (arg == "-q" || arg == "--quiet") {
            quiet = true;
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--no-compress") {
            global_compress = false;
        } else if (arg == "-h" || arg == "--help") {
            show_help();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            show_version();
            return 0;
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            show_help();
            return 1;
        } else {
            project_dir = arg;
        }
    }

    if (project_dir.empty()) {
        std::cerr << "Error: No project directory specified.\n";
        return 1;
    }

    if (!fs::exists(project_dir) || !fs::is_directory(project_dir)) {
        std::cerr << "Error: Provided path is not a valid directory: " << project_dir.string() << "\n";
        return 1;
    }

    if (output_file.empty()) {
        output_file = project_dir.filename().string() + ".obpak";
    }

    if (!quiet) {
        std::cout << "Packing project: " << project_dir.string() << "\n";
        std::cout << "Output file: " << output_file << "\n";
    }

    IO::ContainerWriter writer;
    int success_count = 0;
    int fail_count = 0;

    // recursively scan the project directory
    for (const auto &entry: fs::recursive_directory_iterator(project_dir)) {
        if (entry.is_directory()) continue; // Skip folders

        const fs::path &filepath = entry.path();

        std::string canonical_path = fs::relative(filepath, project_dir).generic_string();
        std::string ext = filepath.extension().string();

        // convert extension to lowercase
        std::ranges::transform(ext, ext.begin(), ::tolower);

        try {
            if (ext == ".obsl") {
                // scripts Lex -> Parse -> AST Serialize
                std::string source_code = read_file_string(filepath);
                ObSL::Lexer lexer(source_code);
                auto tokens = lexer.tokenize();
                ObSL::Parser parser(std::move(tokens));
                auto ast = parser.parse();
                ObSL::ASTSerializer serializer;
                std::vector<uint8_t> binary_blob = serializer.finalize(ast);

                writer.add_compiled_script(canonical_path, std::move(binary_blob), global_compress);
                if (verbose && !quiet) std::cout << "[SCRIPT]  " << canonical_path << "\n";
            } else if (ext == ".json") {
                // JSON -> MsgPack
                std::string source_code = read_file_string(filepath);
                nlohmann::json j = nlohmann::json::parse(source_code);
                std::vector<uint8_t> binary_msgpack = nlohmann::json::to_msgpack(j);

                writer.add_binary_json(canonical_path, std::move(binary_msgpack), global_compress);
                if (verbose && !quiet) std::cout << "[JSON]    " << canonical_path << "\n";
            } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".mp3" || ext == ".ogg") {
                // media is just Raw Bytes, no compression
                std::vector<uint8_t> raw_data = read_file_binary(filepath);
                writer.add_raw_data(canonical_path, std::move(raw_data), IO::Package::EntryType::Media, false);
                if (verbose && !quiet) std::cout << "[MEDIA]   " << canonical_path << "\n";
            } else if (ext == ".vert" || ext == ".frag" || ext == ".glsl") {
                // shaders
                std::vector<uint8_t> raw_data = read_file_binary(filepath);
                writer.add_raw_data(canonical_path, std::move(raw_data), IO::Package::EntryType::ShaderSource,
                                    global_compress);
                if (verbose && !quiet) std::cout << "[SHADER]  " << canonical_path << "\n";
            } else if (ext == ".obmap") {
                // binary generic
                std::vector<uint8_t> raw_data = read_file_binary(filepath);
                writer.add_raw_data(canonical_path, std::move(raw_data), IO::Package::EntryType::RawBinary,
                                    global_compress);
                if (verbose && !quiet) std::cout << "[OBMAP]   " << canonical_path << "\n";
            } else if (filepath.filename() == "imgui.ini" || filepath.filename() == ".DS_Store") {
                // ignore list
                continue;
            } else {
                // fallback
                std::vector<uint8_t> raw_data = read_file_binary(filepath);
                writer.add_raw_data(canonical_path, std::move(raw_data), IO::Package::EntryType::RawBinary,
                                    global_compress);
                if (verbose && !quiet) std::cout << "[RAW]     " << canonical_path << "\n";
            }

            ++success_count;
        } catch (const std::exception &e) {
            std::cerr << "FAIL: " << canonical_path << " - " << e.what() << "\n";
            ++fail_count;
        }
    }

    // Write the container
    if (success_count > 0) {
        try {
            writer.write(output_file);

            if (!quiet) {
                const auto total = success_count + fail_count;
                std::cout << "\nSuccess: Wrote " << output_file << "\n";
                std::cout << "Packed " << success_count << "/" << total << " files.\n";
            }
        } catch (const std::exception &e) {
            std::cerr << "\nFatal: could not write package - " << e.what() << "\n";
            return 1;
        }
    } else {
        std::cerr << "Error: No files were successfully packed.\n";
        return 1;
    }

    return fail_count == 0 ? 0 : 1;
}
