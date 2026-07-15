#pragma once

#include <fstream>
#include <iosfwd>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include "Logger/LoggerService.h"
#include "EntityFactory.h"
#include "IO/VFS/VFS.h"
#include "ECS/Registry.h"
#include "ECS/Components/PrefabSourceComponent.h"

namespace IO {
    class PrefabManager {
    public:
        static ECS::EntityID Instantiate(ECS::Registry &registry, Core::ResourceManager &resources, const std::string &filepath) {
            if (const auto it = s_prefab_cache.find(filepath); it != s_prefab_cache.end()) {
                const ECS::EntityID newId = registry.CreateEntity();
                ECS::Entity newEntity(newId, &registry);
                EntityFactory::DeserializeEntity(newEntity, it->second, resources);
                newEntity.AddComponent<ECS::Components::PrefabSourceComponent>(filepath, it->second);
                return newId;
            }

            std::string_view dataView;
            std::string ownedData;
            if (const auto view = VFS::ReadVirtualView(filepath)) {
                dataView = *view;
            } else if (auto owned = VFS::ReadVirtual(filepath)) {
                ownedData = std::move(*owned);
                dataView = ownedData;
            } else {
                if (auto *logger = Logging::LoggerService::Get()) {
                    logger->log("PrefabManager", "Failed to instantiate: " + filepath + " (Not found in VFS)", Logging::LogSeverity::Error);
                }
                return 0;
            }

            nlohmann::json prefabJson;
            try {
                if (VFS::IsPackaged()) {
                    std::vector<uint8_t> bytes(dataView.begin(), dataView.end());
                    prefabJson = nlohmann::json::from_msgpack(bytes);
                } else {
                    prefabJson = nlohmann::json::parse(dataView);
                }
            } catch (const std::exception &e) {
                if (auto *logger = Logging::LoggerService::Get()) {
                    logger->log("PrefabManager", "Core decoding error for " + filepath + ": " + e.what(), Logging::LogSeverity::Error);
                }
                return 0;
            }


            s_prefab_cache[filepath] = prefabJson;

            const ECS::EntityID newId = registry.CreateEntity();
            ECS::Entity newEntity(newId, &registry);
            EntityFactory::DeserializeEntity(newEntity, prefabJson, resources);
            newEntity.AddComponent<ECS::Components::PrefabSourceComponent>(filepath, prefabJson);
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

            s_prefab_cache[filepath] = std::move(prefabJson);
            return true;
        }

        static std::vector<std::string> GetPrefabFiles() {
            std::vector<std::string> files;
            const auto resolved = VFS::Resolve("assets/prefabs/");
            if (std::filesystem::exists(resolved)) {
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
