#include "SceneSerialization.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "MapSerialization.h"
#include "VFS.h"
#include "Core/ProjectConfig.h"
#include "Core/Utils.h"
#include "IO/AssetLoader.h"
#include "IO/EntityFactory.h"
#include "Scenes/Scene.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Systems/MapRuntimeSystem.h"

namespace IO::SceneIO {
    using json = nlohmann::json;

    void RoundJsonFloats(json &j, const int decimals = 3) {
        const float factor = std::pow(10.f, decimals);
        if (j.is_number_float()) {
            j = std::round(j.get<float>() * factor) / factor;
        } else if (j.is_array()) {
            for (auto &el : j)
                RoundJsonFloats(el, decimals);
        } else if (j.is_object()) {
            for (auto &[k, v] : j.items())
                RoundJsonFloats(v, decimals);
        }
    }

    bool Deserialize(const std::string &path, Scenes::Scene &scene) {
        auto fileData = VFS::ReadVirtual(path);
        if (!fileData.has_value()) {
            std::cerr << "[SceneIO] Failed to open scene through VFS: " << path << "\n";
            return false;
        }

        auto &[ScenePath, Name, BackgroundMusicPath, BackgroundClearColor, AmbientLight] = scene.GetProperties();
        ScenePath = path;

        json j;
        try {
            if (VFS::IsPackaged()) {
                std::vector<uint8_t> bytes(fileData.value().begin(), fileData.value().end());
                j = json::from_msgpack(bytes);
            } else {
                j = json::parse(fileData.value());
            }
        } catch (const std::exception &e) {
            std::cerr << "[SceneIO] Scene parsing failure: " << e.what() << "\n";
            return false;
        }

        if (j.contains("properties")) {
            auto &properties = j["properties"];
            if (properties.contains("name")) {
                Name = properties["name"].get<std::string>();
            }
            if (properties.contains("clear_color")) {
                if (auto &c = properties["clear_color"]; c.is_array() && c.size() >= 4) {
                    BackgroundClearColor = {c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>()};
                } else {
                    std::cerr << "SceneSerializer Warning: 'clear_color' is malformed. Defaulting to black.\n";
                }
            }

            if (properties.contains("background_music")) {
                BackgroundMusicPath = properties["background_music"].get<std::string>();
            }

            if (properties.contains("ambient_light")) {
                AmbientLight = properties["ambient_light"].get<float>();
            }
        }

        auto &resources = *scene.GetContext().resources;
        // factories must be registered before this point!
        //  ASSETS
        if (j.contains("assets")) {
            AssetLoader::LoadAssets(j["assets"], resources);
        }

        if (j.contains("grid") && j["grid"].contains("map_file")) {
            auto mapPath = j["grid"]["map_file"].get<std::string>();

            // map entity via ECS
            ECS::Entity mapEntity(scene.GetRegistry().CreateEntity(), &scene.GetRegistry());
            mapEntity.SetName("MAP");
            ECS::Components::MapComponent mapComp;
            mapComp.mapFilePath = mapPath;

            if (!MapIO::Deserialize(mapPath, mapComp.grid)) {
                std::cerr << "SceneSerializer: Failed to load map file: " << mapPath << "\n";
            }

            // bind visual resources from scene json grid section
            auto &gridJson = j["grid"];
            std::string meshId = gridJson.value("mesh_id", "hex_mesh");
            std::string shaderId = gridJson.value("shader_id", "base_shader");


            auto hexMesh = resources.Get<Rendering::Mesh>(meshId);
            auto shader = resources.Get<Rendering::Shader>(shaderId);
            mapComp.hexMesh = hexMesh;

            if (gridJson.contains("types")) {
                for (auto &gridtypes = gridJson["types"]; auto &typeElement : gridtypes) {
                    uint8_t id = typeElement.value("id", static_cast<uint8_t>(1));
                    std::string textureId = typeElement.value("texture", "hex_tex");

                    auto typeTexture = resources.Get<Rendering::Texture>(textureId);

                    glm::vec4 color(1.0f);
                    if (typeElement.contains("color")) {
                        auto &c = typeElement["color"];
                        color = {c[0], c[1], c[2], c[3]};
                    }

                    Rendering::Material typeMat{shader, typeTexture, color};

                    mapComp.typeMats.emplace(id, typeMat);
                }
            }

            if (shader) {
                mapComp.outlineMat =
                        std::make_shared<Rendering::Material>(Rendering::Material{shader, nullptr, {1, 0, 0, 0.5f}});
                mapComp.pathToMat =
                        std::make_shared<Rendering::Material>(Rendering::Material{shader, nullptr, {1, 1, 1, 0.5f}});
            } else {
                std::cerr << "SceneSerializer: Missing map visual assets!\n";
            }
            mapComp.needsMeshUpdate = true;
            mapComp.lightmap.ambient = AmbientLight;
            mapEntity.AddComponent<ECS::Components::MapComponent>(mapComp);
            mapEntity.AddComponent<ECS::Components::MapStateComponent>();
        }

        // ENTITIES
        if (j.contains("entities")) {
            for (const auto &entityData : j["entities"]) {
                ECS::Entity entity(scene.GetRegistry().CreateEntity(), &scene.GetRegistry());

                EntityFactory::DeserializeEntity(entity, entityData, resources);
            }
        }

        ECS::Systems::MapRuntimeSystem::OnMapChanged(scene.GetRegistry(), scene.GetContext());
        return true;
    }

    //
    // spacer here so i can more easily ending of deserialize into serialize
    //

    bool Serialize(const std::string &path, Scenes::Scene &scene) {
        json j;
        auto &resources = *scene.GetContext().resources;

        // props
        auto &sceneProps = scene.GetProperties();
        j["properties"]["name"] = sceneProps.Name;
        glm::vec4 c = sceneProps.BackgroundClearColor;
        j["properties"]["clear_color"] = {c[0], c[1], c[2], c[3]};
        j["properties"]["background_music"] = sceneProps.BackgroundMusicPath;

        j["properties"]["ambient_light"] = sceneProps.AmbientLight;

        // ecs query to save grid
        scene.GetRegistry().ForEach<ECS::Components::MapComponent>([&](ECS::Entity,
                                                                       const ECS::Components::MapComponent *mapComp) {
            std::string mapFile = mapComp->mapFilePath.empty()
                                          ? Core::PathUtils::Join(Core::MAP_PATH, "unknown", Core::MAP_FILE_EXTENSION)
                                          : mapComp->mapFilePath;

            j["grid"]["map_file"] = mapFile;
            if (!MapIO::Serialize(mapFile, mapComp->grid)) {
                std::cerr << "SceneSerializer: Failed to write map file: " << mapFile << "\n";
            }

            if (mapComp->hexMesh) {
                j["grid"]["mesh_id"] = resources.GetKey<Rendering::Mesh>(mapComp->hexMesh);
            }

            if (!mapComp->typeMats.empty()) {
                j["grid"]["types"] = json::array();

                std::vector<uint8_t> keys;
                for (const auto &key : mapComp->typeMats | std::views::keys)
                    keys.push_back(key);
                std::ranges::sort(keys);

                for (uint8_t id : keys) {
                    const auto &material = mapComp->typeMats.at(id);
                    json typeJson;
                    typeJson["id"] = id;

                    if (material.texture) {
                        typeJson["texture"] = resources.GetKey(material.texture);
                    }

                    if (material.color != glm::vec4(1.0f)) {
                        typeJson["color"] = {material.color.r, material.color.g, material.color.b, material.color.a};
                    }
                    j["grid"]["types"].push_back(typeJson);
                }
            }
        });

        // ASSETS
        j["assets"]["textures"] = json::array();
        j["assets"]["shaders"] = json::array();
        j["assets"]["meshes"] = json::array();
        j["assets"]["materials"] = json::array();

        SerializeAssets(j["assets"]["textures"], resources.GetAll<Rendering::Texture>(),
                        [](const std::string &id, const std::shared_ptr<Rendering::Texture> &tex) {
                            return json{{"id", id}, {"path", tex->GetPath()}};
                        });

        SerializeAssets(
                j["assets"]["shaders"], resources.GetAll<Rendering::Shader>(),
                [](const std::string &id, const std::shared_ptr<Rendering::Shader> &shad) {
                    return json{{"id", id}, {"vertex", shad->GetVertexPath()}, {"fragment", shad->GetFragmentPath()}};
                });

        SerializeAssets(j["assets"]["meshes"], resources.GetAll<Rendering::Mesh>(),
                        [](const std::string &id, const std::shared_ptr<Rendering::Mesh> &mesh) {
                            return json{{"id", id}, {"factory", mesh->GetFactoryId()}};
                        });

        SerializeAssets(j["assets"]["materials"], resources.GetAll<Rendering::Material>(),
                        [&](const std::string &id, const std::shared_ptr<Rendering::Material> &mat) {
                            return json{{"id", id},
                                        {"shader", resources.GetKey(mat->shader)},
                                        {"texture", resources.GetKey(mat->texture)},
                                        {"color", {mat->color.r, mat->color.g, mat->color.b, mat->color.a}}};
                        });

        // ENTITIES
        j["entities"] = json::array();
        for (const auto &livingEntities = scene.GetRegistry().GetLivingEntities();
             ECS::EntityID entityID : livingEntities) {
            ECS::Entity entity(entityID, &scene.GetRegistry());

            if (entity.HasComponent<ECS::Components::MapComponent>()) {
                continue;
            }

            json entityJson;
            EntityFactory::SerializeEntity(entity, entityJson, resources);

            if (!entityJson.empty()) {
                j["entities"].push_back(entityJson);
            }
        }

        RoundJsonFloats(j, 3);
        std::filesystem::path resolvedPath = VFS::Resolve(path);
        std::ofstream file(resolvedPath);
        if (!file.is_open())
            return false;
        file << j.dump(4);
        scene.OnSaved();
        return true;
    }
} // namespace IO::SceneIO
