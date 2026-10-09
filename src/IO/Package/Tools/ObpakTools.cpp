#include "ObpakTools.h"

#include <cctype>
#include <exception>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "AssetPacking.h"
#include "DependencyGraph.h"
#include "FileIO.h"
#include "IgnoreRules.h"

#include "Core/Project.h"
#include "IO/AssetCatalog.h"
#include "IO/AssetCatalogFile.h"
#include "IO/AssetDependencies.h"
#include "IO/Package/Container.h"
#include "IO/VFS/VFS.h"
#include "Logger/LoggerService.h"
#include "Rendering/Types/Shader/Preprocessor/ShaderPreprocessor.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "ObpakTools"

namespace IO::Package::Tools {
    struct ExportManifest {
        nlohmann::json catalog;
        std::set<std::string> files;
    };

    static bool BuildExportManifest(const std::vector<std::string> &scenePaths, const std::filesystem::path &outputDirectory, ExportManifest &manifest) {
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

            std::vector<std::string> pendingScripts;
            std::set<std::string> scannedScenes;
            std::set<std::string> scannedPrefabs;

            const auto projectPath = [&](const std::string &path) { return std::filesystem::relative(VFS::Resolve(VFS::ToRelative(path)), projRoot).generic_string(); };

            const auto queueScript = [&](const std::string &path) {
                if (path.empty()) {
                    return;
                }

                addFile(path);
                const auto relativePath = projectPath(path);

                auto extension = std::filesystem::path(relativePath).extension().string();

                for (char &c : extension) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }

                if (extension == ".obsl") {
                    pendingScripts.push_back(relativePath);
                }
            };

            const auto scanDocument = [&](const std::string &path, bool prefab) {
                if (path.empty()) {
                    throw std::runtime_error("Empty export document path.");
                }

                addFile(path);
                const auto relativePath = projectPath(path);
                auto &scanned = prefab ? scannedPrefabs : scannedScenes;

                if (!scanned.insert(relativePath).second) {
                    return;
                }

                const auto source = VFS::ReadVirtualJson(relativePath);
                if (!source || !source->is_object()) {
                    throw std::runtime_error("Could not read export document: " + relativePath);
                }

                nlohmann::json document;
                if (prefab) {
                    document = {{"entities", nlohmann::json::array({*source})}};
                } else {
                    document = *source;
                }

                auto refs = AssetDependencies::CollectDirectRefs(document);
                required.textures.merge(refs.textures);
                required.shaders.merge(refs.shaders);
                required.meshes.merge(refs.meshes);
                required.materials.merge(refs.materials);
                required.fonts.merge(refs.fonts);
                required.animationSets.merge(refs.animationSets);

                if (const auto properties = document.find("properties"); properties != document.end() && properties->is_object()) {
                    addFile(properties->value("background_music", std::string{}));
                }

                if (const auto grid = document.find("grid"); grid != document.end() && grid->is_object()) {
                    addFile(grid->value("map_file", std::string{}));
                }

                const auto entities = document.find("entities");
                if (entities == document.end() || !entities->is_array()) {
                    return;
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
                        queueScript(script->at("scriptPath").get<std::string>());
                    } else if (const auto paths = script->find("scriptPaths"); paths != script->end() && paths->is_array()) {
                        for (const auto &scriptPath : *paths) {
                            queueScript(scriptPath.get<std::string>());
                        }
                    }
                }
            };

            for (const auto &scenePath : scenePaths) {
                scanDocument(scenePath, false);
            }

            const auto *catalogAssets = AssetCatalog::GetAssets();
            if (!catalogAssets) {
                throw std::runtime_error("No asset catalog loaded.");
            }

            const auto scriptRoot = projRoot / "assets" / "scripts";
            std::set<std::string> scannedScripts;
            std::set<std::string> keepEntireTypes;
            bool keepAllCatalogAssets = false;
            bool includedProjectFiles = false;

            const auto includeProjectFiles = [&]() {
                if (includedProjectFiles) {
                    return;
                }
                includedProjectFiles = true;
                keepAllCatalogAssets = true;

                const auto ignore = IgnoreRules::ForProject(projRoot);
                const auto output = std::filesystem::weakly_canonical(outputDirectory);
                const auto root = std::filesystem::weakly_canonical(projRoot);

                for (auto it = std::filesystem::recursive_directory_iterator(projRoot); it != std::filesystem::recursive_directory_iterator(); ++it) {
                    const auto &entry = *it;
                    const auto path = entry.path();
                    if (entry.is_directory()) {
                        const auto name = path.filename();
                        if (ignore.IsIgnored(path, true) || name == ".git" || name == ".hg" || name == ".svn" || (output != root && std::filesystem::weakly_canonical(path) == output)) {
                            it.disable_recursion_pending();
                        }
                        continue;
                    }
                    if (!entry.is_regular_file() || ignore.IsIgnored(path, false)) {
                        continue;
                    }

                    const auto relativePath = projectPath(path.generic_string());
                    // The catalog is generated below; previous packages are never input assets.
                    if (relativePath == "assets.json" || path.extension() == ".obpak") {
                        continue;
                    }
                    queueScript(relativePath);
                }
            };

            for (std::size_t index = 0; index < pendingScripts.size(); ++index) {
                const auto scriptPath = pendingScripts[index];

                if (!scannedScripts.insert(scriptPath).second) {
                    continue;
                }

                ScriptAssetAnalysis analysis;
                const auto dependencies = CollectScriptDependencies(VFS::Resolve(scriptPath), projRoot, scriptRoot, *catalogAssets, analysis);

                for (const auto &call : analysis.unrecognizedCalls) {
                    LOG_INFO(LOG_WHO, "Keeping full asset catalog: " + scriptPath + " contains unclassified call: " + call);
                }

                if (!analysis.unresolvedFileCalls.empty()) {
                    for (const auto &call : analysis.unresolvedFileCalls) {
                        LOG_INFO(LOG_WHO, "Keeping project files and full asset catalog: dynamic path for " + call + " in script: " + scriptPath);
                    }
                    includeProjectFiles();
                }

                for (const auto &file : analysis.files) {
                    addFile(file);
                }

                for (const auto &scenePath : analysis.scenes) {
                    scanDocument(scenePath, false);
                }

                for (const auto &prefabPath : analysis.prefabs) {
                    scanDocument(prefabPath, true);
                }

                required.textures.merge(analysis.required.textures);
                required.shaders.merge(analysis.required.shaders);
                required.meshes.merge(analysis.required.meshes);
                required.materials.merge(analysis.required.materials);
                required.fonts.merge(analysis.required.fonts);
                required.animationSets.merge(analysis.required.animationSets);

                keepAllCatalogAssets |= analysis.includeAllCatalogAssets;
                keepEntireTypes.insert(analysis.dynamicAssets.begin(), analysis.dynamicAssets.end());

                for (const auto &dependency : dependencies) {
                    queueScript(dependency);
                }
            }

            const std::pair<const char *, std::set<std::string> *> targets[] = {{"textures", &required.textures}, {"shaders", &required.shaders}, {"meshes", &required.meshes}, {"materials", &required.materials},
                    {"fonts", &required.fonts}, {"animation_sets", &required.animationSets}};

            for (const auto &type : keepEntireTypes) {
                if (!catalogAssets->contains(type)) {
                    throw std::runtime_error("Unknown dynamic asset type: " + type);
                }
            }

            for (const auto &[type, ids] : targets) {
                if (keepAllCatalogAssets || keepEntireTypes.contains(type)) {
                    for (const auto &asset : catalogAssets->at(type)) {
                        ids->insert(asset.at("id").get<std::string>());
                    }
                }
            }

            if (keepAllCatalogAssets) {
                LOG_INFO("ObpakTools", "Scripts require keeping the full asset catalog.");
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

            // shaders
            Rendering::ShaderPreprocessor preprocessor;
            preprocessor.setVirtualPathMode(true);
            const auto engineShaderDir = GetInternalsDirectory() / "resources" / "shaders";

            preprocessor.setFileLoader([&](const std::filesystem::path &path) -> std::string {
                const auto resolved = VFS::Resolve(path);
                if (!resolved.empty() && std::filesystem::is_regular_file(resolved)) {
                    addFile(path.generic_string());
                    return read_file_string(resolved);
                }

                const auto enginePath = engineShaderDir / path.filename();
                if (std::filesystem::is_regular_file(enginePath)) {
                    return read_file_string(enginePath);
                }

                throw std::runtime_error("Required shader include is missing: " + path.generic_string());
            });

            for (const auto &shader : result.catalog.at("assets").at("shaders")) {
                for (const char *type : {"vertex", "fragment"}) {
                    const auto path = shader.value(type, std::string{});
                    if (path.empty()) {
                        continue;
                    }

                    const auto virtualPath = VFS::ToRelative(path);
                    const auto src = read_file_string(VFS::Resolve(virtualPath));

                    Rendering::BuiltinPPState state;
                    preprocessor.processSource(src, virtualPath, state);
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
        if (out.empty()) {
            out = "game";
        }
        return out;
    }

    bool PackageCurrentProject(const std::string &output_dir) {
        namespace fs = std::filesystem;
        try {
            const auto projectDir = VFS::GetProjectRoot();
            if (projectDir.empty() || !fs::is_directory(projectDir)) {
                throw std::runtime_error("No valid project directory mounted.");
            }
            if (output_dir.empty()) {
                throw std::runtime_error("No export directory specified.");
            }

            const auto projectJson = VFS::ReadVirtualJson("project.json");
            if (!projectJson || !projectJson->is_object()) {
                throw std::runtime_error("Could not read project.json.");
            }

            std::set<std::string> scenes;
            const auto startScene = projectJson->value("start_scene", std::string{});

            if (!startScene.empty()) {
                scenes.insert(VFS::ToRelative(startScene));
            }

            const auto sceneDir = VFS::Resolve("assets/scenes");
            if (!sceneDir.empty() && fs::is_directory(sceneDir)) {
                for (const auto &entry : fs::recursive_directory_iterator(sceneDir)) {
                    if (!entry.is_regular_file()) {
                        continue;
                    }

                    auto extension = entry.path().extension().string();
                    for (char &c : extension) {
                        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    }

                    if (extension == ".json") {
                        scenes.insert(VFS::ToRelative(entry.path()));
                    }
                }
            }

            const std::vector<std::string> scenePaths(scenes.begin(), scenes.end());
            ExportManifest manifest;
            if (!BuildExportManifest(scenePaths, std::filesystem::absolute(output_dir), manifest)) {
                return false;
            }

            manifest.files.erase("assets.json");

            const auto ignoreRules = IgnoreRules::ForProject(projectDir);
            const auto checkRequiredFile = [&](const std::string &path) {
                const fs::path relativePath(path);

                if (ignoreRules.IsIgnored(projectDir / relativePath, false)) {
                    throw std::runtime_error("Required export file is ignored: " + path);
                }

                for (auto directory = relativePath.parent_path(); !directory.empty(); directory = directory.parent_path()) {
                    if (ignoreRules.IsIgnored(projectDir / directory, true)) {
                        throw std::runtime_error("Required export file is inside an ignored "
                                                 "directory: " +
                                                 path);
                    }
                }
            };

            checkRequiredFile("assets.json");
            for (const auto &path : manifest.files) {
                checkRequiredFile(path);
            }

            ContainerWriter writer;
            DependencyGraph dependencies;
            PackOptions options{.global_compress = true, .verbose = true, .quiet = false, .binary_name = "obliberry exporter"};

            writer.add_binary_json("assets.json", nlohmann::json::to_msgpack(manifest.catalog), options.global_compress);

            std::size_t packedFiles = 1;
            const auto scriptRoot = projectDir / "assets" / "scripts";

            for (const auto &path : manifest.files) {
                if (!pack_one_file(VFS::Resolve(path), projectDir, scriptRoot, writer, dependencies, options)) {
                    throw std::runtime_error("Could not pack: " + path);
                }

                ++packedFiles;
            }

            const auto engineShaderDir = GetInternalsDirectory() / "resources" / "shaders";

            if (fs::is_directory(engineShaderDir)) {
                for (const auto &entry : fs::directory_iterator(engineShaderDir)) {
                    if (!entry.is_regular_file()) {
                        continue;
                    }

                    const auto virtualPath = "engine/shaders/" + entry.path().filename().generic_string();

                    writer.add_raw_data(virtualPath, read_file_binary(entry.path()), EntryType::ShaderSource, options.global_compress);

                    ++packedFiles;
                }
            }

            if (!dependencies.validate(options.binary_name)) {
                throw std::runtime_error("Script dependency validation failed.");
            }

            fs::create_directories(output_dir);
            const auto outFile = fs::path(output_dir) / "data.obpak";
            writer.write(outFile);

            LOG_INFO(LOG_WHO, "Wrote " + outFile.string() + " (" + std::to_string(packedFiles) + " files)");

            return true;
        } catch (const std::exception &error) {
            LOG_ERROR(LOG_WHO, std::string("Could not package project: ") + error.what());
            return false;
        }
    }

    void ExportGame(const std::string &output_dir) {
        std::cout << "Exporting game to: " << output_dir << "\n";

        if (!Core::Project::GetActive()) {
            LOG_ERROR(LOG_WHO, "No active project.");
            return;
        }

        if (!PackageCurrentProject(output_dir)) {
            return;
        }

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
                LOG_ERROR(LOG_WHO, "Ensure obliberry_runtime is built and located in "
                                   "the 'internal' folder next to the editor");
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
