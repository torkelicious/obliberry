#include "ObpakTools.h"
#include <exception>
#include <filesystem>
#include "IO/AssetCatalog.h"
#include "IO/AssetCatalogFile.h"
#include "IO/AssetDependencies.h"
#include "Logger/LoggerService.h"
#include <iostream>
#include "AssetPacking.h"
#include "DependencyGraph.h"
#include "FileIO.h"
#include "IgnoreRules.h"
#include <set>
#include <stdexcept>
#include <string>
#include <cctype>
#include <vector>
#include <utility>
#include "Core/Project.h"
#include "IO/VFS/VFS.h"
#include "IO/Package/Container.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "ObpakTools"

namespace IO::Package::Tools {

    struct ExportManifest {
        nlohmann::json catalog;
        std::set<std::string> files; // paths
    };

    static bool BuildExportManifest(const std::vector<std::string> &scenePaths, ExportManifest &manifest) {
        try {
            const auto projRoot = VFS::GetProjectRoot();
            if (projRoot.empty() || scenePaths.empty()) {
                throw std::runtime_error("No project or export scenes specified");
            }

            if (!AssetCatalog::IsLoaded() && !AssetCatalog::Load()) {
                return false;
            }

            ExportManifest result;
            AssetDependencies::RequiredAssets required;

            const auto addFile = [&](const std::string &path) {
                if (path.empty()) {
                    return;
                }

                const auto resolved = VFS::Resolve(VFS::ToRelative(path));
                if (resolved.empty() || !std::filesystem::is_regular_file(resolved)) {
                    throw std::runtime_error("Required export file is missing or invalid: " + path);
                }

                result.files.insert(std::filesystem::relative(resolved, projRoot).generic_string());
            };

            addFile("project.json");

            for (const auto &scenePath : scenePaths) {
                addFile(scenePath);
                const auto scene = VFS::ReadVirtualJson(VFS::ToRelative(scenePath));
                if (!scene || !scene->is_object()) {
                    throw std::runtime_error("Could not read export scene: " + scenePath);
                }

                auto refs = AssetDependencies::CollectDirectRefs(*scene);
                required.textures.merge(refs.textures);
                required.shaders.merge(refs.shaders);
                required.meshes.merge(refs.meshes);
                required.materials.merge(refs.materials);
                required.fonts.merge(refs.fonts);
                required.animationSets.merge(refs.animationSets);

                if (const auto properties = scene->find("properties"); properties != scene->end() && properties->is_object()) {
                    addFile(properties->value("background_music", std::string{}));
                }

                if (const auto grid = scene->find("grid"); grid != scene->end() && grid->is_object()) {
                    addFile(grid->value("map_file", std::string{}));
                }

                const auto entities = scene->find("entities");
                if (entities == scene->end() || !entities->is_array()) {
                    continue;
                }

                for (const auto &entity : *entities) {
                    if (!entity.is_object()) {
                        continue;
                    }

                    const auto components = entity.find("components");
                    if (components == entity.end() || !components->is_object()) {
                        continue;
                    }

                    const auto script = components->find("ScriptComponent");
                    if (script == components->end() || !script->is_object()) {
                        continue;
                    }

                    if (script->contains("scriptPath")) {
                        addFile(script->at("scriptPath").get<std::string>());
                    } else if (const auto paths = script->find("scriptPaths"); paths != script->end() && paths->is_array()) {
                        for (const auto &path : *paths) {
                            addFile(path.get<std::string>());
                        }
                    }
                }
            }

            if (!AssetDependencies::ResolveDependencies(required)) {
                return false;
            }

            nlohmann::json subset;
            if (!AssetDependencies::BuildAssetSubset(required, subset)) {
                return false;
            }

            result.catalog = IO::CatalogFile::Empty();
            result.catalog["assets"] = std::move(subset);

            for (const auto &[type, entries] : result.catalog.at("assets").items()) {
                for (const auto &asset : entries) {
                    if (type == "textures" || type == "fonts" || type == "animation_sets") {
                        const auto path = asset.at("path").get<std::string>();
                        if (path.empty()) {
                            throw std::runtime_error("Empty file path for asset: " + asset.at("id").get<std::string>());
                        }
                        addFile(path);
                    } else if (type == "shaders") {
                        addFile(asset.value("vertex", std::string{}));

                        const auto fragment = asset.at("fragment").get<std::string>();
                        if (fragment.empty()) {
                            throw std::runtime_error("Empty fragment path for shader: " + asset.at("id").get<std::string>());
                        }
                        addFile(fragment);
                    }
                }
            }

            manifest = std::move(result);
            return true;

        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, std::string("Could not build export manifest: ") + e.what());
            return false;
        }
    }

    static std::string SanitizeExecutableName(const std::string &input) {
        std::string out;
        bool lastWasUnderscore = false;
        for (const unsigned char c : input) {
            if (std::isalnum(c) || c == '-' || c == '_') {
                out += c;
                lastWasUnderscore = false;
            } else if (!lastWasUnderscore) {
                out += '_';
                lastWasUnderscore = true;
            }
        }
        if (out.empty())
            out = "game";
        return out;
    }


    void PackageCurrentProject(const std::string &output_dir) {
        std::filesystem::path project_dir = VFS::GetProjectRoot();

        std::filesystem::path out_file = "data.obpak";
        const std::string BINARY_NAME = "obliberry exporter";


        if (project_dir.empty()) {
            LOG_ERROR(LOG_WHO, "No project directory specified");
            return;
        }
        if (!std::filesystem::exists(project_dir) || !std::filesystem::is_directory(project_dir)) {
            LOG_ERROR(LOG_WHO, "Provided path is not a valid directory: " + project_dir.string());
            return;
        }
        out_file = std::filesystem::path(output_dir) / out_file;

        std::cout << "Packing project: " + project_dir.string() << "\n";
        std::cout << "Output file: " + out_file.string() << "\n";

        std::filesystem::path script_root = project_dir / "assets" / "scripts";
        if (!std::filesystem::exists(script_root)) {
            std::cout << "Expected scripts folder not found: " + script_root.string() << "\n";
            return;
        }

        ContainerWriter writer;
        DependencyGraph dep_graph;
        IgnoreRules ignore_rules = IgnoreRules::ForProject(project_dir);
        PackOptions opts{.global_compress = true, .verbose = true, .quiet = false, .binary_name = BINARY_NAME};
        opts.ignore = &ignore_rules;

        int success_count = 0, fail_count = 0;

        for (std::filesystem::recursive_directory_iterator it(project_dir), end_it; it != end_it; ++it) {
            if (it->is_directory()) {
                // prune ignored directories
                if (ignore_rules.IsIgnored(it->path(), /*is_dir=*/true))
                    it.disable_recursion_pending();
                continue;
            }
            try {
                if (pack_one_file(it->path(), project_dir, script_root, writer, dep_graph, opts))
                    ++success_count;
            } catch (const std::exception &e) {
                LOG_ERROR(LOG_WHO, it->path().string() + " - " + e.what());
                ++fail_count;
            }
        }

        // engine shader helpers inside the package so shader #includes work properly in exported games (VFS prefix engine/shaders/)
        if (const std::filesystem::path engineShaderDir = GetInternalsDirectory() / "resources" / "shaders"; std::filesystem::exists(engineShaderDir)) {
            for (const auto &entry : std::filesystem::directory_iterator(engineShaderDir)) {
                if (!entry.is_regular_file())
                    continue;
                try {
                    auto raw = read_file_binary(entry.path());
                    const std::string virtPath = "engine/shaders/" + entry.path().filename().generic_string();
                    writer.add_raw_data(virtPath, std::move(raw), EntryType::ShaderSource, opts.global_compress);
                    ++success_count;
                    LOG_INFO(LOG_WHO, "[ENGINE_SHADER] " + virtPath);
                } catch (const std::exception &e) {
                    LOG_ERROR(LOG_WHO, entry.path().string() + " - " + e.what());
                    ++fail_count;
                }
            }
        }

        if (!dep_graph.validate(BINARY_NAME)) {
            LOG_WARN(LOG_WHO, "Dependency validation warnings reported (packing will continue)");
        }

        if (success_count > 0) {
            try {
                writer.write(out_file);
                std::cout << "Wrote " + out_file.string() << "\n";
                std::cout << "Packed " + std::to_string(success_count) + "/" + std::to_string(success_count + fail_count) + " files.\n";
            } catch (const std::exception &e) {
                LOG_ERROR(LOG_WHO, std::string("Could not write package - ") + e.what());
            }
        } else {
            LOG_ERROR(LOG_WHO, "No valid assets found to pack");
        }
    }

    void ExportGame(const std::string &output_dir) {
        std::cout << "Exporting game to: " << output_dir << "\n";
        PackageCurrentProject(output_dir);

        const std::string clean_project_name = SanitizeExecutableName(Core::Project::GetActive()->GetConfig().Title);
#ifdef _WIN32
        const std::string runtime_name = "obliberry_runtime.exe";
        const std::string export_name = clean_project_name + ".exe";
#else
        const std::string runtime_name = "obliberry_runtime";
        const std::string &export_name = clean_project_name;
#endif
        // Locate the runtime
        const std::filesystem::path runtime_src = GetInternalsDirectory() / runtime_name;
        const std::filesystem::path dest_exe = std::filesystem::path(output_dir) / export_name;
        const std::filesystem::path project_dir = VFS::GetProjectRoot();
        try {
            if (std::filesystem::exists(runtime_src)) {
                std::filesystem::copy_file(runtime_src, dest_exe, std::filesystem::copy_options::overwrite_existing);
                LOG_INFO(LOG_WHO, "Export Successfully copied runtime binary to " + dest_exe.string());
            } else {
                LOG_ERROR(LOG_WHO, "Error: Could not find runtime binary at " + runtime_src.string());
                LOG_ERROR(LOG_WHO, "Ensure obliberry_runtime is built and located in the 'internal' folder next to the editor");
            }
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Exception while copying runtime: " + std::string(e.what()));
        }
        const std::filesystem::path outPath(output_dir);
        try {
            if (std::filesystem::exists(project_dir / "graphics.json")) {
                std::filesystem::copy_file(project_dir / "graphics.json", outPath / "graphics.json", std::filesystem::copy_options::overwrite_existing);
                LOG_INFO(LOG_WHO, "Copied graphics.json to export directory");
            }
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to copy graphics.json: " + std::string(e.what()));
        }
    }
} // namespace IO::Package::Tools
#pragma pop_macro("LOG_WHO")
