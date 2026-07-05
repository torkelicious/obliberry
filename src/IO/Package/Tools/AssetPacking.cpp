#include "AssetPacking.h"
#include "FileIO.h"
#include "CliCommon.h"
#include <ObSL/Lexer.h>
#include <ObSL/Parser.h>
#include <ObSL/ASTSerializer.h>
#include <nlohmann/json.hpp>
#include <algorithm>

namespace IO::Package::Tools {
    static std::string lower_ext(const std::filesystem::path &p) {
        std::string ext = p.extension().string();
        std::ranges::transform(ext, ext.begin(), ::tolower);
        return ext;
    }

    bool pack_one_file(const std::filesystem::path &filepath, const std::filesystem::path &project_dir,
                       IO::ContainerWriter &writer, DependencyGraph &dep_graph, const PackOptions &opts) {
        std::string canonical_path = std::filesystem::relative(filepath, project_dir).generic_string();
        std::string ext = lower_ext(filepath);

        if (filepath.filename() == "imgui.ini" || filepath.filename() == ".DS_Store") {
            return true; // silently skipped not a failure
        }

        if (ext == ".obsl") {
            std::string source_code = read_file_string(filepath);
            ObSL::Lexer lexer(source_code);
            auto tokens = lexer.tokenize();
            ObSL::Parser parser(std::move(tokens));
            auto ast = parser.parse();

            dep_graph.add_script(canonical_path, ast, project_dir);

            ObSL::ASTSerializer serializer;
            std::vector<uint8_t> binary_blob = serializer.finalize(ast);
            writer.add_compiled_script(canonical_path, std::move(binary_blob), opts.global_compress);
            if (opts.verbose && !opts.quiet) log_info(opts.binary_name, "[SCRIPT]  " + canonical_path);
        } else if (ext == ".json") {
            std::string source_code = read_file_string(filepath);
            nlohmann::json j = nlohmann::json::parse(source_code);
            std::vector<uint8_t> binary_msgpack = nlohmann::json::to_msgpack(j);
            writer.add_binary_json(canonical_path, std::move(binary_msgpack), opts.global_compress);
            if (opts.verbose && !opts.quiet) log_info(opts.binary_name, "[JSON]    " + canonical_path);
        } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".mp3" || ext == ".ogg") {
            auto raw_data = read_file_binary(filepath);
            writer.add_raw_data(canonical_path, std::move(raw_data), IO::Package::EntryType::Media, false);
            if (opts.verbose && !opts.quiet) log_info(opts.binary_name, "[MEDIA]   " + canonical_path);
        } else if (ext == ".vert" || ext == ".frag" || ext == ".glsl") {
            auto raw_data = read_file_binary(filepath);
            writer.add_raw_data(canonical_path, std::move(raw_data), IO::Package::EntryType::ShaderSource,
                                opts.global_compress);
            if (opts.verbose && !opts.quiet) log_info(opts.binary_name, "[SHADER]  " + canonical_path);
        } else if (ext == ".obmap") {
            auto raw_data = read_file_binary(filepath);
            writer.add_raw_data(canonical_path, std::move(raw_data), IO::Package::EntryType::RawBinary,
                                opts.global_compress);
            if (opts.verbose && !opts.quiet) log_info(opts.binary_name, "[OBMAP]   " + canonical_path);
        } else {
            auto raw_data = read_file_binary(filepath);
            writer.add_raw_data(canonical_path, std::move(raw_data), IO::Package::EntryType::RawBinary,
                                opts.global_compress);
            if (opts.verbose && !opts.quiet) log_info(opts.binary_name, "[RAW]     " + canonical_path);
        }

        return true;
    }
} // namespace IO::Package::Tools
