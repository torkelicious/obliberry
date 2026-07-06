#include "AssetPacking.h"
#include "FileIO.h"
#include "CliCommon.h"
#include <ObSL/Lexer.h>
#include <ObSL/Parser.h>
#include <ObSL/ASTSerializer.h>
#include <ObSL/ModulePath.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <algorithm>

namespace IO::Package::Tools {
    const std::filesystem::path ignorelist[] = {
        "imgui.ini",
        ".DS_Store",
    };

    static std::string lower_ext(const std::filesystem::path &p) {
        std::string ext = p.extension().string();
        std::ranges::transform(ext, ext.begin(), tolower);
        return ext;
    }

    // NOLINTBEGIN (*-pro-type-static-cast-downcast)
    static void resolve_and_collect_using_paths(
        ObSL::Stmt *stmt,
        const std::filesystem::path &script_root,
        const std::filesystem::path &project_dir,
        std::vector<std::string> &out_deps
    ) {
        if (!stmt) return;
        using ObSL::StmtType;
        switch (stmt->type()) {
            case StmtType::Using: {
                auto *using_stmt = static_cast<ObSL::UsingStmt *>(stmt);
                std::string canonical = ObSL::canonicalize_module_path(script_root, using_stmt->path);
                if (!std::filesystem::exists(canonical)) {
                    std::string alt = ObSL::canonicalize_module_path(project_dir, using_stmt->path);
                    if (std::filesystem::exists(alt))
                        canonical = std::move(alt);
                }
                const std::string project_relative =
                        std::filesystem::relative(canonical, project_dir).generic_string();
                using_stmt->path = project_relative;
                out_deps.push_back(project_relative);
                break;
            }
            case StmtType::Block:
                for (auto &s: static_cast<ObSL::BlockStmt *>(stmt)->statements)
                    resolve_and_collect_using_paths(s.get(), script_root, project_dir, out_deps);
                break;
            case StmtType::If: {
                const auto *n = static_cast<ObSL::IfStmt *>(stmt);
                resolve_and_collect_using_paths(n->then_branch.get(), script_root, project_dir, out_deps);
                resolve_and_collect_using_paths(n->else_branch.get(), script_root, project_dir, out_deps);
                break;
            }
            case StmtType::While:
                resolve_and_collect_using_paths(
                    static_cast<ObSL::WhileStmt *>(stmt)->body.get(), script_root, project_dir, out_deps);
                break;
            case StmtType::Foreach:
                resolve_and_collect_using_paths(
                    static_cast<ObSL::ForeachStmt *>(stmt)->body.get(), script_root, project_dir, out_deps);
                break;
            case StmtType::Function:
                resolve_and_collect_using_paths(
                    static_cast<ObSL::FunctionStmt *>(stmt)->body.get(), script_root, project_dir, out_deps);
                break;
            case StmtType::Switch:
                for (auto &c: static_cast<ObSL::SwitchStmt *>(stmt)->cases)
                    for (auto &s: c.statements)
                        resolve_and_collect_using_paths(s.get(), script_root, project_dir, out_deps);
                break;
            case StmtType::TryCatch: {
                const auto *n = static_cast<ObSL::TryCatchStmt *>(stmt);
                resolve_and_collect_using_paths(n->try_body.get(), script_root, project_dir, out_deps);
                resolve_and_collect_using_paths(n->catch_body.get(), script_root, project_dir, out_deps);
                break;
            }
            default:
                break;
        }
    }

    // NOLINTEND (*-pro-type-static-cast-downcast)

    bool pack_one_file(const std::filesystem::path &filepath, const std::filesystem::path &project_dir,
                       const std::filesystem::path &script_root,
                       ContainerWriter &writer, DependencyGraph &dep_graph, const PackOptions &opts) {
        std::string canonical_path = std::filesystem::relative(filepath, project_dir).generic_string();
        std::string ext = lower_ext(filepath);

        if (std::ranges::find(ignorelist, filepath.filename()) != std::end(ignorelist)) {
            return true; // silently skipped not a failure
        }

        if (ext == ".obsl") {
            std::string source_code = read_file_string(filepath);
            ObSL::Lexer lexer(source_code);
            auto tokens = lexer.tokenize();
            ObSL::Parser parser(std::move(tokens));
            auto ast = parser.parse();

            std::vector<std::string> deps;
            for (auto &stmt: ast)
                resolve_and_collect_using_paths(stmt.get(), script_root, project_dir, deps);
            dep_graph.add_script(canonical_path, std::move(deps));

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
            writer.add_raw_data(canonical_path, std::move(raw_data), EntryType::Media, false);
            if (opts.verbose && !opts.quiet) log_info(opts.binary_name, "[MEDIA]   " + canonical_path);
        } else if (ext == ".vert" || ext == ".frag" || ext == ".glsl") {
            auto raw_data = read_file_binary(filepath);
            writer.add_raw_data(canonical_path, std::move(raw_data), EntryType::ShaderSource,
                                opts.global_compress);
            if (opts.verbose && !opts.quiet) log_info(opts.binary_name, "[SHADER]  " + canonical_path);
        } else if (ext == ".obmap") {
            auto raw_data = read_file_binary(filepath);
            writer.add_raw_data(canonical_path, std::move(raw_data), EntryType::RawBinary,
                                opts.global_compress);
            if (opts.verbose && !opts.quiet) log_info(opts.binary_name, "[OBMAP]   " + canonical_path);
        } else {
            auto raw_data = read_file_binary(filepath);
            writer.add_raw_data(canonical_path, std::move(raw_data), EntryType::RawBinary,
                                opts.global_compress);
            if (opts.verbose && !opts.quiet) log_info(opts.binary_name, "[RAW]     " + canonical_path);
        }

        return true;
    }
} // namespace IO::Package::Tools
