#pragma once

#include <fstream>
#include <iosfwd>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include "EntityFactory.h"
#include "VFS.h"
#include "ECS/Registry.h"

namespace IO {
    class PrefabManager {
    public:
        static ECS::EntityID Instantiate(ECS::Registry &registry, Core::ResourceManager &resources,
                                         const std::string &filepath) {
            if (const auto it = s_prefab_cache.find(filepath); it != s_prefab_cache.end()) {
                const ECS::EntityID newId = registry.CreateEntity();
                ECS::Entity newEntity(newId, &registry);
                EntityFactory::DeserializeEntity(newEntity, it->second, resources);
                return newId;
            }

            auto fileData = VFS::ReadVirtual(filepath);
            if (!fileData.has_value()) {
                std::cerr << "[PrefabManager] Failed to instantiate: " << filepath
                        << " (Not found in VFS)\n";
                return 0;
            }

            nlohmann::json prefabJson;
            try {
                if (VFS::IsPackaged()) {
                    std::vector<uint8_t> bytes(fileData.value().begin(), fileData.value().end());
                    prefabJson = nlohmann::json::from_msgpack(bytes);
                } else {
                    prefabJson = nlohmann::json::parse(fileData.value());
                }
            } catch (const std::exception &e) {
                std::cerr << "[PrefabManager] Core decoding error for " << filepath << ": " << e.what() << "\n";
                return 0;
            }


            s_prefab_cache[filepath] = prefabJson;

            const ECS::EntityID newId = registry.CreateEntity();
            ECS::Entity newEntity(newId, &registry);
            EntityFactory::DeserializeEntity(newEntity, prefabJson, resources);
            return newId;
        }

        static void ClearCache() {
            s_prefab_cache.clear();
        }

        static void UnloadPrefab(const std::string &filepath) {
            s_prefab_cache.erase(filepath);
        }

    private:
        inline static std::unordered_map<std::string, nlohmann::json> s_prefab_cache;
    };
} // namespace IO
