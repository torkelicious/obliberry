#include "SceneSerialization.h"
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

    void RoundJsonFloats(json &j, int decimals = 3) {
        float factor = std::pow(10.f, decimals);
        if (j.is_number_float()) {
            j = std::round(j.get<float>() * factor) / factor;
        } else if (j.is_array()) {
            for (auto &el: j)
                RoundJsonFloats(el, decimals);
        } else if (j.is_object()) {
            for (auto &[k, v]: j.items())
                RoundJsonFloats(v, decimals);
        }
    }

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
            if (!MapIO::Serialize(mapFile, mapComp->grid)) {
                std::cerr << "SceneSerializer: Failed to write map file: " << mapFile << "\n";
            }

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

        // ASSETS
        j["assets"]["textures"] = json::array();
        j["assets"]["shaders"] = json::array();
        j["assets"]["meshes"] = json::array();
        j["assets"]["materials"] = json::array();

        SerializeAssets(
            j["assets"]["textures"],
            resources.GetAll<Texture>(),
            [](const std::string &id, const std::shared_ptr<Texture> &tex) {
                return json{
                    {"id", id},
                    {"path", tex->GetPath()}
                };
            }
        );

        SerializeAssets(
            j["assets"]["shaders"],
            resources.GetAll<Shader>(),
            [](const std::string &id, const std::shared_ptr<Shader> &shad) {
                return json{
                    {"id", id},
                    {"vertex", shad->GetVertexPath()},
                    {"fragment", shad->GetFragmentPath()}
                };
            }
        );

        SerializeAssets(
            j["assets"]["meshes"],
            resources.GetAll<Mesh>(),
            [](const std::string &id, const std::shared_ptr<Mesh> &mesh) {
                return json{
                    {"id", id},
                    {"factory", mesh->GetFactoryId()}
                };
            }
        );

        SerializeAssets(
            j["assets"]["materials"],
            resources.GetAll<Material>(),
            [&](const std::string &id, const std::shared_ptr<Material> &mat) {
                return json{
                    {"id", id},
                    {"shader", resources.GetKey(mat->shader)},
                    {"texture", resources.GetKey(mat->texture)},
                    {
                        "color", {
                            mat->color.r,
                            mat->color.g,
                            mat->color.b,
                            mat->color.a
                        }
                    }
                };
            }
        );


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

        RoundJsonFloats(j, 3);
        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << j.dump(4);
        return true;
    }
}
