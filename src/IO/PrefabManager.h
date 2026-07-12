#pragma once

#include <fstream>
#include <iosfwd>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include "Core/LoggerService.h"
#include "EntityFactory.h"
#include "VFS.h"
#include "ECS/Registry.h"

namespace IO {
    class PrefabManager {
    public:
        static ECS::EntityID Instantiate(ECS::Registry &registry, Core::ResourceManager &resources, const std::string &filepath) {
            if (const auto it = s_prefab_cache.find(filepath); it != s_prefab_cache.end()) {
                const ECS::EntityID newId = registry.CreateEntity();
                ECS::Entity newEntity(newId, &registry);
                EntityFactory::DeserializeEntity(newEntity, it->second, resources);
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
                if (auto *logger = Core::Logging::LoggerService::Get()) {
                    logger->log("PrefabManager", "Failed to instantiate: " + filepath + " (Not found in VFS)", Core::Logging::LogSeverity::Error);
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
                if (auto *logger = Core::Logging::LoggerService::Get()) {
                    logger->log("PrefabManager", "Core decoding error for " + filepath + ": " + e.what(), Core::Logging::LogSeverity::Error);
                }
                return 0;
            }


            s_prefab_cache[filepath] = prefabJson;

            const ECS::EntityID newId = registry.CreateEntity();
            ECS::Entity newEntity(newId, &registry);
            EntityFactory::DeserializeEntity(newEntity, prefabJson, resources);
            return newId;
        }

        static void ClearCache() { s_prefab_cache.clear(); }

        static void UnloadPrefab(const std::string &filepath) { s_prefab_cache.erase(filepath); }

    private:
        inline static std::unordered_map<std::string, nlohmann::json> s_prefab_cache;
    };
} // namespace IO
