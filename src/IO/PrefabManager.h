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

            // parse from disk if not cached
            std::filesystem::path resolvedPath = VFS::Resolve(filepath);
            std::ifstream file(resolvedPath);
            if (!file.is_open()) {
                std::cerr << "[PrefabManager] Failed to instantiate: " << filepath
                        << " (Resolved: " << resolvedPath.string() << ")\n";
                return 0;
            }


            nlohmann::json prefabJson;
            file >> prefabJson;
            s_prefab_cache[filepath] = prefabJson;

            ECS::EntityID newId = registry.CreateEntity();
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
