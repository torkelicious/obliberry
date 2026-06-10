#include "SceneSerializer.h"
#include <fstream>
#include <iostream>

#include "json.hpp"
#include "MapSerialization.h"
#include "Core/Utils.h"
#include "Renderer/MeshFactory.h"
#include "IO/AssetLoader.h"
#include "IO/EntityFactory.h"
#include "Scenes/Scene.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Systems/MapRuntimeSystem.h"

namespace SceneIO {
    using json = nlohmann::json;

    bool Deserialize(const std::string &path, Scene &scene) {
        std::ifstream file(path);

        if (!file.is_open())
            return false;

        json j;
        file >> j;

        auto &resources = *scene.GetContext().resources;

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
            Entity mapEntity(scene.GetRegistry().CreateEntity(), &scene.GetRegistry());
            MapComponent mapComp;
            mapComp.mapFilePath = mapPath;

            if (!MapIO::Deserialize(mapPath, mapComp.grid)) {
                std::cerr << "SceneSerializer: Failed to load map file: " << mapPath << "\n";
            }

            // bind visual resources from scene json grid section
            auto &gridJson = j["grid"];
            std::string meshId = gridJson.value("mesh_id", "hex_mesh");
            std::string shaderId = gridJson.value("shader_id", "base_shader");
            std::string grassTexId = gridJson.value("grass_tex_id", "grass_tex");
            std::string sandTexId = gridJson.value("sand_tex_id", "sand_tex");

            auto hexMesh = resources.Get<Mesh>(meshId);
            auto shader = resources.Get<Shader>(shaderId);
            auto grassTex = resources.Get<Texture>(grassTexId);
            auto sandTex = resources.Get<Texture>(sandTexId);

            mapComp.hexMesh = hexMesh;
            if (shader && grassTex && sandTex && hexMesh) {
                mapComp.grassMat = {shader, grassTex, {1, 1, 1, 1}};
                mapComp.sandMat = {shader, sandTex, {1, 1, 1, 1}};
                mapComp.outlineMat = {shader, nullptr, {1, 0, 0, 0.5f}};
                mapComp.pathToMat = {shader, nullptr, {1, 1, 1, 0.5f}};
            } else {
                std::cerr << "SceneSerializer: Missing map visual assets!\n";
            }
            mapComp.needsMeshUpdate = true;
            mapEntity.AddComponent<MapComponent>(mapComp);
            mapEntity.AddComponent<MapStateComponent>();
        }

        // ENTITIES
        if (j.contains("entities")) {
            for (const auto &entityData: j["entities"]) {
                Entity entity(
                    scene.GetRegistry().CreateEntity(),
                    &scene.GetRegistry()
                );

                EntityFactory::DeserializeEntity(entity, entityData, resources);
            }
        }

        MapRuntimeSystem::OnMapChanged(scene.GetRegistry());
        return true;
    }

    bool Serialize(const std::string &path, Scene &scene) {
        json j;
        auto &resources = *scene.GetContext().resources;

        // ecs query to save grid
        scene.GetRegistry().ForEach<MapComponent>([&](Entity, MapComponent *mapComp) {
            std::string mapFile = mapComp->mapFilePath.empty()
                                      ? PathUtils::Join(MAP_PATH, "unknown", MAP_FILE_EXTENSION)
                                      : mapComp->mapFilePath;
            j["grid"]["map_file"] = mapFile;
            MapIO::Serialize(mapFile, mapComp->grid);

            if (mapComp->hexMesh) {
                j["grid"]["mesh_id"] = resources.GetKey<Mesh>(mapComp->hexMesh);
            }
            if (mapComp->grassMat.shader) {
                j["grid"]["shader_id"] = resources.GetKey<Shader>(mapComp->grassMat.shader);
            }
            if (mapComp->grassMat.texture) {
                j["grid"]["grass_tex_id"] = resources.GetKey<Texture>(mapComp->grassMat.texture);
            }
            if (mapComp->sandMat.texture) {
                j["grid"]["sand_tex_id"] = resources.GetKey<Texture>(mapComp->sandMat.texture);
            }
        });

        // ENTITIES
        j["entities"] = json::array();
        const auto &livingEntities = scene.GetRegistry().GetLivingEntities();

        for (EntityID entityID: livingEntities) {
            Entity entity(entityID, &scene.GetRegistry());

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
