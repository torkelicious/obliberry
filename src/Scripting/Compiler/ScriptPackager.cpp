#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdint>

#include <ObSL/Lexer.h>
#include <ObSL/Parser.h>
#include <ObSL/ScriptEntry.h>

#include "ASTSerializer.h"
#include "IO/Package/Container.h"


const std::string TITLE_NAME = "ObSL-Working-Name-Script-Packager";
const std::string BINARY_NAME = "obsl_pack";
constexpr float VERSION = 1.0;

namespace fs = std::filesystem;

// Helpers

static std::string read_file(const fs::path &filepath) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filepath.string());
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static void show_help() {
    std::cout <<
            TITLE_NAME <<
            " package .obsl scripts as .obpak packages\n"
            "\n"
            "Usage: " << BINARY_NAME << " [options] <input.obsl> [input.obsl...]\n"
            "\n"
            "Options:\n"
            "  -o, --output <file>    Output .obpak path\n"
            "                         (default: <input>.obpak for single files)\n"
            "  --ast                  Also emit standalone .ast binary files\n"
            "  --no-compress          Disable LZ4 compression\n"
            "  -v, --verbose          Show per-file packaging details\n"
            "  -q, --quiet            Suppress all non-error output\n"
            "  -h, --help             Show this help\n"
            "  --version              Show version information\n";
}


int main(int argc, char *argv[]) {
    std::string output_file;
    std::vector<fs::path> input_files;
    bool emit_ast = false;
    bool compress = true;
    bool verbose = false;
    bool quiet = false;

    for (int i = 1; i < argc; ++i) {
        if (const std::string arg = argv[i]; arg == "-o" || arg == "--output") {
            if (i + 1 >= argc) {
                std::cerr << "error: --output requires a filename argument.\n";
                return 1;
            }
            output_file = argv[++i];
        } else if (arg == "--emit-ast" || arg == "--ast") {
            emit_ast = true;
        } else if (arg == "--no-compress") {
            compress = false;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-q" || arg == "--quiet") {
            quiet = true;
        } else if (arg == "-h" || arg == "--help") {
            show_help();
            return 0;
        } else if (arg == "--version") {
            std::cout << TITLE_NAME << VERSION << "\n";
            return 0;
        } else {
            input_files.emplace_back(arg);
        }
    }

    if (input_files.empty()) {
        show_help();
        return 0;
    }

    // derive default output (use a copy so replace_extension doesn't mutate the original path)
    if (output_file.empty()) {
        if (input_files.size() == 1) {
            auto default_path = input_files[0];
            output_file = default_path.replace_extension(".obpak").string();
        } else {
            std::cerr << "error: Multiple input files require --output <file>.\n";
            return 1;
        }
    }

    // compile
    IO::ContainerWriter writer;
    int success_count = 0;
    int fail_count = 0;

    if (!quiet) {
        std::cout << "packaging " << input_files.size()
                << (input_files.size() == 1 ? " script" : " scripts")
                << "->" << fs::path(output_file).filename().string();
        if (!compress) std::cout << "  [no compression]";
        std::cout << "\n";
    }

    for (const auto &filepath: input_files) {
        try {
            const std::string source_code = read_file(filepath);

            ObSL::Lexer lexer(source_code);
            auto tokens = lexer.tokenize();

            ObSL::Parser parser(std::move(tokens));
            auto ast = parser.parse();

            ObSL::ASTSerializer serializer;
            auto binary_blob = serializer.finalize(ast);

            // emit standalone .ast if requested
            if (emit_ast) {
                auto ast_path = filepath;
                ast_path.replace_extension(".ast");
                std::ofstream out_ast(ast_path, std::ios::binary);
                out_ast.write(reinterpret_cast<const char *>(binary_blob.data()),
                              static_cast<std::streamsize>(binary_blob.size()));

                if (verbose && !quiet) {
                    std::cout << "ast: " << ast_path.filename().string()
                            << "  (" << binary_blob.size() << " bytes)\n";
                }
            }

            // stage into the package
            writer.add_compiled_script(filepath.generic_string(),
                                       std::move(binary_blob), compress);

            if (verbose && !quiet) {
                std::cout << "DONE:" << filepath.filename().string() << "\n";
            }

            ++success_count;
        } catch (const std::exception &e) {
            std::cerr << "FAIL:" << filepath.filename().string()
                    << " - " << e.what() << "\n";
            ++fail_count;
        }
    }

    // write the container
    if (success_count > 0) {
        try {
            writer.write(output_file);

            if (!quiet) {
                const auto total = success_count + fail_count;
                std::cout << "ok wrote " << output_file
                        << "  (" << success_count << "/" << total << " succeeded"
                        << (fail_count ? ", " + std::to_string(fail_count) + " failed" : "")
                        << ")\n";
            }
        } catch (const std::exception &e) {
            std::cerr << "\nFatal: could not write package - " << e.what() << "\n";
            return 1;
        }
    }
    return fail_count == 0 ? 0 : 1;
}
