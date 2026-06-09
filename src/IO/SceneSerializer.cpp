#include "SceneSerializer.h"
#include <fstream>
#include <iostream>

#include "json.hpp"
#include "MapSerialization.h"
#include "Core/Utils.h"
#include "ECS/ECS.h"
#include "Renderer/MeshFactory.h"
#include "IO/AssetLoader.h"
#include "IO/EntityFactory.h"
#include "Scenes/Scene.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"

namespace SceneIO {
    using json = nlohmann::json;

    bool Deserialize(const std::string &path, Scene &scene) {
        std::ifstream file(path);

        if (!file.is_open())
            return false;

        json j;
        file >> j;

        auto &resources = *scene.m_Context.resources;

        // register once before loading !!!
        AssetLoader::RegisterMeshFactory("Quad", []() {
            auto data = MeshFactory::CreateQuad();
            return std::make_shared<Mesh>(data.vertices, data.indices);
        });

        AssetLoader::RegisterMeshFactory("PointTopHex", []() {
            auto data = MeshFactory::CreatePointTopHex(0.5f);
            return std::make_shared<Mesh>(data.vertices, data.indices);
        });

        //  ASSETS
        if (j.contains("assets")) {
            AssetLoader::LoadAssets(j["assets"], resources);
        }

        if (j.contains("grid") && j["grid"].contains("map_file")) {
            std::string mapPath = j["grid"]["map_file"].get<std::string>();

            // map entity via ECS
            Entity mapEntity(scene.m_Registry.CreateEntity(), &scene.m_Registry);
            MapComponent mapComp;
            mapComp.mapFilePath = mapPath;

            if (!MapIO::Deserialize(mapPath, mapComp.grid)) {
                std::cerr << "SceneSerializer: Failed to load map file: " << mapPath << "\n";
            }

            mapEntity.AddComponent<MapComponent>(mapComp);
            mapEntity.AddComponent<MapStateComponent>();
        }

        // ENTITIES
        if (j.contains("entities")) {
            for (const auto &entityData: j["entities"]) {
                Entity entity(
                    scene.m_Registry.CreateEntity(),
                    &scene.m_Registry
                );

                EntityFactory::DeserializeEntity(entity, entityData, resources);
            }
        }

        return true;
    }

    bool Serialize(const std::string &path, Scene &scene) {
        json j;
        auto &resources = *scene.m_Context.resources;

        // ecs query to save grid
        scene.m_Registry.ForEach<MapComponent>([&](Entity, MapComponent *mapComp) {
            std::string mapFile = mapComp->mapFilePath.empty()
                                      ? PathUtils::Join(SCENE_PATH, "unkown.obmap")
                                      : mapComp->mapFilePath;
            j["grid"]["map_file"] = mapFile;
            MapIO::Serialize(mapFile, mapComp->grid);
        });

        // ENTITIES
        j["entities"] = json::array();
        const auto &livingEntities = scene.m_Registry.GetLivingEntities();

        for (EntityID entityID: livingEntities) {
            Entity entity(entityID, &scene.m_Registry);

            if (entity.HasComponent<MapComponent>()) {
                continue;
            }

            json entityJson;
            EntityFactory::SerializeEntity(entity, entityJson, resources);

            if (!entityJson.empty()) {
                j["entities"].push_back(entityJson);
            }
        }

        std::ofstream file(path);
        if (!file.is_open()) return false;

        file << j.dump(4);
        return true;
    }
}
