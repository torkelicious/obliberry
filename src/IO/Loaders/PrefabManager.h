#pragma once

#include <fstream>
#include <iosfwd>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "Core/ResourceManager.h"
#include "ECS/Types.h"
#include "IO/Loaders/SceneAssetLoader.h"
#include "Logger/LoggerService.h"
#include "EntityFactory.h"
#include "IO/VFS/VFS.h"
#include "ECS/Registry.h"
#include "ECS/Components/PrefabSourceComponent.h"
#include "Scenes/Scene.h"
#include "nlohmann/json_fwd.hpp"

namespace IO {
    class PrefabManager {
    public:
        static ECS::EntityID Instantiate(Scenes::Scene &scene, const std::string &filepath) {

            auto *resources = scene.GetContext().resources;

            if (!resources) {
                return ECS::INVALID_ENTITY_ID;
            }

            auto it = s_prefab_cache.find(filepath);
            if (it == s_prefab_cache.end()) {
                auto read = VFS::ReadVirtualJson(filepath);
                if (!read.has_value() || !read->is_object()) {
                    return ECS::INVALID_ENTITY_ID;
                }
                it = s_prefab_cache.emplace(filepath, std::move(*read)).first;
            }

            const auto &prefabJson = it->second;
            const nlohmann::json references = {{"entities", nlohmann::json::array({prefabJson})}};

            SceneAssetLoader::SceneAssetScope scope;
            if (!SceneAssetLoader::LoadReferenced(references, scope)) {
                return ECS::INVALID_ENTITY_ID;
            }

            auto &registry = scene.GetRegistry();
            const ECS::EntityID newId = registry.CreateEntity();
            ECS::Entity newEntity(newId, &registry);

            EntityFactory::DeserializeEntity(newEntity, prefabJson, *resources);
            newEntity.AddComponent<ECS::Components::PrefabSourceComponent>(filepath, prefabJson);
            scene.AddAssetScope(std::move(scope));

            return newId;
        }

        static bool SavePrefab(ECS::Entity &entity, const std::string &filepath, Core::ResourceManager &resources) {
            nlohmann::json prefabJson;
            EntityFactory::SerializeEntity(entity, prefabJson, resources);

            // PrefabSourceComponent is just  metadata and not part of the prefab template
            if (prefabJson.contains("components") && prefabJson["components"].contains("PrefabSourceComponent")) {
                prefabJson["components"].erase("PrefabSourceComponent");
            }

            const auto resolved = VFS::Resolve(filepath);
            std::ofstream out(resolved);
            if (!out) {
                if (auto *logger = Logging::LoggerService::Get()) {
                    logger->log("PrefabManager", "Failed to save prefab to: " + filepath, Logging::LogSeverity::Error);
                }
                return false;
            }
            out << prefabJson.dump(4);
            out.close();
            if (!out) {
                if (auto *logger = Logging::LoggerService::Get()) {
                    logger->log("PrefabManager", "Failed to write prefab to: " + filepath, Logging::LogSeverity::Error);
                }
                return false;
            }

            s_prefab_cache[filepath] = std::move(prefabJson);
            return true;
        }

        static std::vector<std::string> GetPrefabFiles() {
            std::vector<std::string> files;
            if (const auto resolved = VFS::Resolve("assets/prefabs/"); std::filesystem::exists(resolved)) {
                for (const auto &entry : std::filesystem::directory_iterator(resolved)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".json") {
                        auto relPath = std::filesystem::relative(entry.path(), VFS::GetProjectRoot());
                        std::string relStr = relPath.string();
                        for (auto &c : relStr)
                            if (c == '\\')
                                c = '/';
                        files.push_back(std::move(relStr));
                    }
                }
            }
            return files;
        }

        static void ClearCache() { s_prefab_cache.clear(); }

        static void UnloadPrefab(const std::string &filepath) { s_prefab_cache.erase(filepath); }

    private:
        inline static std::unordered_map<std::string, nlohmann::json> s_prefab_cache;
    };
} // namespace IO
