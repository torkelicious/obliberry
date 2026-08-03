#include "SceneSerialization.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include "Logger/LoggerService.h"
#include "MapSerialization.h"
#include "VFS/VFS.h"
#include "Config/ProjectConfig.h"
#include "Core/Utils/PathUtils.h"
#include "Loaders/AssetLoader.h"
#include "Loaders/EntityFactory.h"
#include "Scenes/Scene.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/RelationshipComponent.h"
#include "ECS/Systems/MapRuntimeSystem.h"
#include "IO/Loaders/UISerializer.h"
#include "UI/Rendering/UISystem.h"
#include "UI/Text/Font.h"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "SceneIO"

namespace IO::SceneIO {
    using json = nlohmann::json;

    bool Deserialize(const std::string &path, Scenes::Scene &scene) {
        std::string_view dataView;
        std::string ownedData;
        if (auto view = VFS::ReadVirtualView(path)) {
            dataView = *view;
        } else if (auto owned = VFS::ReadVirtual(path)) {
            ownedData = std::move(*owned);
            dataView = ownedData;
        } else {
            LOG_ERROR(LOG_WHO, "Failed to open scene through VFS: " + path);
            return false;
        }

        auto &[ScenePath, Name, BackgroundMusicPath, BackgroundClearColor, AmbientLight] = scene.GetProperties();
        ScenePath = VFS::ToRelative(path);

        json j;
        try {
            if (VFS::IsPackaged()) {
                std::vector<uint8_t> bytes(dataView.begin(), dataView.end());
                j = json::from_msgpack(bytes);
            } else {
                j = json::parse(dataView);
            }
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Scene parsing failure: " + std::string(e.what()));
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
                    LOG_WARN(LOG_WHO, "'clear_color' is malformed. Defaulting to black");
                }
            }

            if (properties.contains("background_music")) {
                BackgroundMusicPath = VFS::ToRelative(properties["background_music"].get<std::string>());
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
            mapComp.mapFilePath = VFS::ToRelative(mapPath);

            if (!MapIO::Deserialize(mapPath, mapComp.grid)) {
                LOG_ERROR(LOG_WHO, "Failed to load map file: " + mapPath);
            }

            // bind visual resources from scene json grid section
            auto &gridJson = j["grid"];
            std::string meshId = gridJson.value("mesh_id", "[Engine] Hex");

            auto hexMesh = resources.Get<Rendering::Mesh>(meshId);
            auto shader = resources.Get<Rendering::Shader>("[Engine] Base");
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

                    mapComp.typeMats.emplace_back(id, std::make_shared<Rendering::Material>(Rendering::Material{shader, typeTexture, color}));
                }
            }

            if (shader) {
                mapComp.outlineMat = std::make_shared<Rendering::Material>(Rendering::Material{shader, nullptr, {1, 0, 0, 0.5f}});
                mapComp.pathToMat = std::make_shared<Rendering::Material>(Rendering::Material{shader, nullptr, {1, 1, 1, 0.5f}});
            } else {
                LOG_ERROR(LOG_WHO, "Missing map visual assets!");
            }
            mapComp.needsMeshUpdate = true;
            mapComp.lightmap.ambient = AmbientLight;

            {
                std::vector<uint8_t> definedTypeIds;
                definedTypeIds.reserve(mapComp.typeMats.size());
                for (const auto &[id, _] : mapComp.typeMats)
                    definedTypeIds.push_back(id);

                for (const auto &[coords, tile] : mapComp.grid.tiles) {
                    if (std::ranges::find(definedTypeIds, tile.type) == definedTypeIds.end()) {
                        LOG_WARN(LOG_WHO, "Tile at (" + std::to_string(coords.q) + "," + std::to_string(coords.r) + ") uses type " + std::to_string(tile.type) + " which has no defined material — it will not render");
                        break; // only log once per load
                    }
                }
            }

            mapEntity.AddComponent<ECS::Components::MapComponent>(mapComp);
            mapEntity.AddComponent<ECS::Components::MapStateComponent>();
        }

        // ENTITIES
        if (j.contains("entities")) {
            std::vector<ECS::EntityID> deserializedIds;
            deserializedIds.reserve(j["entities"].size());

            for (const auto &entityData : j["entities"]) {
                ECS::Entity entity(scene.GetRegistry().CreateEntity(), &scene.GetRegistry());
                EntityFactory::DeserializeEntity(entity, entityData, resources);
                deserializedIds.push_back(static_cast<ECS::EntityID>(entity));
            }

            for (size_t i = 0; i < deserializedIds.size(); ++i) {
                if (const auto &entityData = j["entities"][i]; entityData.contains("parent")) {
                    if (const size_t parentIndex = entityData["parent"].get<size_t>(); parentIndex < deserializedIds.size() && parentIndex != i) {
                        scene.GetRegistry().Reparent(deserializedIds[i], deserializedIds[parentIndex]);
                    }
                }
            }
        }

        // UI
        if (j.contains("ui")) {
            if (auto *uiSys = scene.GetContext().uiSystem) {
                UISerializer::Deserialize(j["ui"], *uiSys, resources);
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
        scene.GetRegistry().ForEach<ECS::Components::MapComponent>([&](ECS::Entity, const ECS::Components::MapComponent *mapComp) {
            if (mapComp->mapFilePath.empty()) {
                LOG_WARN(LOG_WHO, "Map has no file path, skipping grid serialization. Use Save As to assign a path.");
                return;
            }

            j["grid"]["map_file"] = mapComp->mapFilePath;
            if (!MapIO::Serialize(mapComp->mapFilePath, mapComp->grid)) {
                LOG_ERROR(LOG_WHO, "Failed to write map file: " + mapComp->mapFilePath);
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
                    auto it = std::ranges::find_if(mapComp->typeMats, [id](const auto &p) { return p.first == id; });
                    if (it == mapComp->typeMats.end())
                        continue;
                    const auto &material = it->second;
                    json typeJson;
                    typeJson["id"] = id;

                    if (material->texture) {
                        typeJson["texture"] = resources.GetKey(material->texture);
                    }

                    if (material->color != glm::vec4(1.0f)) {
                        typeJson["color"] = {material->color.r, material->color.g, material->color.b, material->color.a};
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
        j["assets"]["fonts"] = json::array();

        SerializeAssets(j["assets"]["textures"], resources.GetAll<Rendering::Texture>(), [](const std::string &id, const std::shared_ptr<Rendering::Texture> &tex) { return json{{"id", id}, {"path", tex->GetPath()}}; });

        SerializeAssets(
                j["assets"]["shaders"], resources.GetAll<Rendering::Shader>(),
                [](const std::string &id, const std::shared_ptr<Rendering::Shader> &shad) { return json{{"id", id}, {"vertex", shad->GetVertexPath()}, {"fragment", shad->GetFragmentPath()}}; }, IsUserAsset);

        SerializeAssets(j["assets"]["meshes"], resources.GetAll<Rendering::Mesh>(),
                        [](const std::string &id, const std::shared_ptr<Rendering::Mesh> &mesh) { return json{{"id", id}, {"factory", mesh->GetFactoryId()}}; });

        SerializeAssets(j["assets"]["materials"], resources.GetAll<Rendering::Material>(), [&](const std::string &id, const std::shared_ptr<Rendering::Material> &mat) {
            return json{{"id", id},
                        {"shader", mat->shader ? resources.GetKey(mat->shader) : "[Engine] Base"},
                        {"texture", resources.GetKey(mat->texture)},
                        {"color", {mat->color.r, mat->color.g, mat->color.b, mat->color.a}}};
        });

        UISerializer::Serialize(j, scene.GetUISystem(), resources);

        SerializeAssets(j["assets"]["fonts"], resources.GetAll<UI::Font>(), [&](const std::string &id, const std::shared_ptr<UI::Font> &font) {
            return json{{"id", id}, {"path", font->GetPath()}, {"size", font->GetFontSize()}, {"sdf", font->IsSDF()}, {"spread", font->GetSDFSpread()}};
        });

        // ENTITIES
        j["entities"] = json::array();
        std::unordered_map<ECS::EntityID, size_t> entityIdToIndex;
        {
            size_t idx = 0;
            for (const ECS::EntityID entityID : scene.GetRegistry().GetLivingEntities()) {
                if (!scene.GetRegistry().IsValid(entityID))
                    continue;
                if (ECS::Entity entity(entityID, &scene.GetRegistry()); entity.HasComponent<ECS::Components::MapComponent>())
                    continue;
                entityIdToIndex[entityID] = idx++;
            }
        }

        for (const ECS::EntityID entityID : scene.GetRegistry().GetLivingEntities()) {
            if (!scene.GetRegistry().IsValid(entityID))
                continue;
            ECS::Entity entity(entityID, &scene.GetRegistry());

            if (entity.HasComponent<ECS::Components::MapComponent>()) {
                continue;
            }

            json entityJson;
            EntityFactory::SerializeEntity(entity, entityJson, resources);

            if (const auto *rel = scene.GetRegistry().GetComponent<ECS::Components::RelationshipComponent>(entityID)) {
                if (rel->parent != ECS::INVALID_ENTITY_ID) {
                    if (auto it = entityIdToIndex.find(rel->parent); it != entityIdToIndex.end()) {
                        entityJson["parent"] = it->second;
                    }
                }
            }

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
#pragma pop_macro("LOG_WHO")
