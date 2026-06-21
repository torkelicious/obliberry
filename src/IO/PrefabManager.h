#ifndef OBLIBERRY_PREFABMANAGER_H
#define OBLIBERRY_PREFABMANAGER_H
#include <fstream>
#include <iosfwd>
#include <json.hpp>

#include "EntityFactory.h"
#include "Core/ResourceManager.h"
#include "ECS/Registry.h"

class PrefabManager {
public:
    static EntityID Instantiate(Registry &registry, ResourceManager &resources, const std::string &filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "failed to instanciate from file";
            return 0;
        }
        nlohmann::json prefabJson;
        file >> prefabJson;
        EntityID newId = registry.CreateEntity();
        Entity newEntity(newId, &registry);
        EntityFactory::DeserializeEntity(newEntity, prefabJson, resources);
        return newId;
    }
};


#endif //OBLIBERRY_PREFABMANAGER_H
