#ifndef OBLIBERRY_PREFABMANAGER_H
#define OBLIBERRY_PREFABMANAGER_H
#include <fstream>
#include <iosfwd>
#include <iostream>
#include <json.hpp>
#include <string>
#include <unordered_map>
#include "EntityFactory.h"
#include "ECS/Registry.h"

class PrefabManager {
public:
    static EntityID Instantiate(Registry &registry, ResourceManager &resources, const std::string &filepath) {
        if (const auto it = s_prefab_cache.find(filepath); it != s_prefab_cache.end()) {
            // cache hit
            const EntityID newId = registry.CreateEntity();
            Entity newEntity(newId, &registry);
            EntityFactory::DeserializeEntity(newEntity, it->second, resources);
            return newId;
        }
        // parse from disk if not cached
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[PrefabManager] Failed to instantiate from file: " << filepath << "\n";
            return 0;
        }

        nlohmann::json prefabJson;
        file >> prefabJson;
        s_prefab_cache[filepath] = prefabJson;

        EntityID newId = registry.CreateEntity();
        Entity newEntity(newId, &registry);
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


#endif //OBLIBERRY_PREFABMANAGER_H
