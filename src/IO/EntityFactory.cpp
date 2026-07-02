#include "EntityFactory.h"
#include <iostream>

#include "ECS/ECS.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/BillboardTagComponent.h"
#include "Rendering/Mesh.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/ScriptComponent.h"

std::unordered_map<std::string, ComponentDeserializer> IO::EntityFactory::s_Deserializers;
std::unordered_map<std::string, ComponentSerializer> IO::EntityFactory::s_Serializers;

const std::unordered_map<std::string, ComponentDeserializer> &IO::EntityFactory::GetDeserializers() {
    return s_Deserializers;
}

const std::unordered_map<std::string, ComponentSerializer> &IO::EntityFactory::GetSerializers() {
    return s_Serializers;
}

void IO::EntityFactory::RegisterDeserializers() {
    if (!s_Deserializers.empty()) return;

    // TRANSFORM COMPONENT
    s_Deserializers["TransformComponent"] = [](ECS::Entity &entity, const nlohmann::json &data,
                                               Core::ResourceManager &/*resources*/) {
        ECS::Components::TransformComponent tc;
        if (data.contains("position")) {
            tc.transform.SetPosition({data["position"][0], data["position"][1], data["position"][2]});
        }
        if (data.contains("scale")) {
            tc.transform.SetScale({data["scale"][0], data["scale"][1], data["scale"][2]});
        }
        entity.AddComponent<ECS::Components::TransformComponent>(tc);
    };

    // MOVEMENT COMPONENT
    s_Deserializers["MovementComponent"] = [](ECS::Entity &entity, const nlohmann::json &data,
                                              Core::ResourceManager &/*resources*/) {
        ECS::Components::MovementComponent mc;
        if (data.contains("timePerStep")) {
            mc.timePerStep = data["timePerStep"].get<float>();
        }
        entity.AddComponent<ECS::Components::MovementComponent>(mc);
    };

    // PLAYER INPUT COMPONENT
    //s_Deserializers["PlayerInputComponent"] = [
    //        ](Entity &entity, const nlohmann::json &data, ResourceManager &resources) {
    //            PlayerInputComponent input;
    //            if (data.contains("LeftClick")) input.LeftClick = data["LeftClick"].get<int>();
    //            if (data.contains("RightClick")) input.RightClick = data["RightClick"].get<int>();
    //            if (data.contains("Up")) input.Up = data["Up"].get<int>();
    //            if (data.contains("Down")) input.Down = data["Down"].get<int>();
    //            if (data.contains("Left")) input.Left = data["Left"].get<int>();
    //            if (data.contains("Right")) input.Right = data["Right"].get<int>();
    //            if (data.contains("Quit")) input.Quit = data["Quit"].get<int>();
    //            entity.AddComponent<PlayerInputComponent>(input);
    //        };

    // MESH COMPONENT
    s_Deserializers["MeshComponent"] = [](ECS::Entity &entity, const nlohmann::json &data,
                                          Core::ResourceManager &resources) {
        ECS::Components::MeshComponent mc;
        if (data.contains("mesh_id")) {
            const std::string meshId = data["mesh_id"].get<std::string>();
            mc.mesh = resources.Get<Rendering::Mesh>(meshId);
            if (!mc.mesh) std::cerr << "EntityFactory: Failed to find Mesh ID '" << meshId << "'\n";
        }
        entity.AddComponent<ECS::Components::MeshComponent>(mc);
    };

    // MATERIAL COMPONENT
    s_Deserializers["MaterialComponent"] = [](ECS::Entity &entity, const nlohmann::json &data,
                                              Core::ResourceManager &resources) {
        ECS::Components::MaterialComponent matComp;
        if (data.contains("material_id")) {
            const std::string matID = data["material_id"].get<std::string>();
            matComp.material = resources.Get<Rendering::Material>(matID);
            if (!matComp.material) std::cerr << "EntityFactory: Failed to find Material ID '" << matID << "'\n";
        }
        entity.AddComponent<ECS::Components::MaterialComponent>(matComp);
    };

    s_Deserializers["BillboardTagComponent"] = [
            ](ECS::Entity &entity, const nlohmann::json &, Core::ResourceManager &) {
                entity.AddComponent<ECS::Components::BillboardTagComponent>();
            };

    // DIRECTIONAL TEXTURE COMPONENT
    s_Deserializers["DirectionalTextureComponent"] = [](ECS::Entity &entity, const nlohmann::json &data,
                                                        Core::ResourceManager &resources) {
        ECS::Components::DirectionalTextureComponent dirTex;
        if (data.contains("textures") && data["textures"].is_array()) {
            auto &texArray = data["textures"];
            for (size_t i = 0; i < 6 && i < texArray.size(); ++i) {
                auto texID = texArray[i].get<std::string>();
                dirTex.textures[i] = resources.Get<Rendering::Texture>(texID);
            }
        }
        if (data.contains("index")) {
            dirTex.index = data["index"].get<int>();
        }
        entity.AddComponent<ECS::Components::DirectionalTextureComponent>(dirTex);
    };

    // POINT LIGHT COMPONENT
    s_Deserializers["PointLightComponent"] = [](ECS::Entity &entity, const nlohmann::json &data,
                                                Core::ResourceManager &) {
        ECS::Components::PointLightComponent plc;
        if (data.contains("color")) {
            plc.color = {data["color"][0], data["color"][1], data["color"][2]};
        }
        if (data.contains("radius")) {
            plc.radius = data["radius"].get<float>();
        }
        if (data.contains("intensity")) {
            plc.intensity = data["intensity"].get<float>();
        }
        entity.AddComponent<ECS::Components::PointLightComponent>(plc);
    };

    // SCRIPT COMPONENT
    s_Deserializers["ScriptComponent"] = [
            ](ECS::Entity &entity, const nlohmann::json &data, Core::ResourceManager &/*resources*/) {
                auto &[scriptPaths, instance_envs, on_update_functions, on_destroy_functions, on_exit_functions,
                    isInitialized,
                    source_codes, ast_nodes, lastModified] = entity.AddComponent<ECS::Components::ScriptComponent>();

                if (data.contains("scriptPath")) {
                    // Single script
                    scriptPaths.push_back(data["scriptPath"].get<std::string>());
                } else if (data.contains("scriptPaths") && data["scriptPaths"].is_array()) {
                    // Multiple scripts
                    for (const auto &scriptPath: data["scriptPaths"]) {
                        scriptPaths.push_back(scriptPath.get<std::string>());
                    }
                }

                // Initialize vectors to match the number of scripts
                const size_t scriptCount = scriptPaths.size();
                // outer vectors: one inner vector per script (inner sized per-worker during InitializeScript)
                instance_envs.resize(scriptCount);
                on_update_functions.resize(scriptCount);
                on_destroy_functions.resize(scriptCount);
                on_exit_functions.resize(scriptCount);
                isInitialized.resize(scriptCount, false);
                source_codes.resize(scriptCount);
                ast_nodes.resize(scriptCount);
                lastModified.resize(scriptCount);
            };
}

void IO::EntityFactory::RegisterSerializers() {
    if (!s_Serializers.empty()) return;

    // TRANSFORM COMPONENT
    s_Serializers["TransformComponent"] = [
            ](const ECS::Entity &entity, nlohmann::json &data, Core::ResourceManager &/*resources*/) {
                if (entity.HasComponent<ECS::Components::TransformComponent>()) {
                    const auto *tc = entity.GetComponent<ECS::Components::TransformComponent>();
                    auto pos = tc->transform.GetPosition();
                    auto scale = tc->transform.GetScale();
                    data["TransformComponent"]["position"] = {pos.x, pos.y, pos.z};
                    data["TransformComponent"]["scale"] = {scale.x, scale.y, scale.z};
                }
            };

    // MOVEMENT COMPONENT
    s_Serializers["MovementComponent"] = [
            ](const ECS::Entity &entity, nlohmann::json &data, Core::ResourceManager &/*resources*/) {
                if (entity.HasComponent<ECS::Components::MovementComponent>()) {
                    auto *mc = entity.GetComponent<ECS::Components::MovementComponent>();
                    data["MovementComponent"]["timePerStep"] = mc->timePerStep;
                    // state data is ignored
                }
            };

    // PLAYER INPUT COMPONENT
    //s_Serializers["PlayerInputComponent"] = [](const Entity &entity, nlohmann::json &data, ResourceManager &resources) {
    //    if (entity.HasComponent<PlayerInputComponent>()) {
    //        auto *pic = entity.GetComponent<PlayerInputComponent>();
    //        data["PlayerInputComponent"]["LeftClick"] = pic->LeftClick;
    //        data["PlayerInputComponent"]["RightClick"] = pic->RightClick;
    //        data["PlayerInputComponent"]["Up"] = pic->Up;
    //        data["PlayerInputComponent"]["Down"] = pic->Down;
    //        data["PlayerInputComponent"]["Left"] = pic->Left;
    //        data["PlayerInputComponent"]["Right"] = pic->Right;
    //        data["PlayerInputComponent"]["Quit"] = pic->Quit;
    //    }
    //};

    // MESH COMPONENT
    s_Serializers["MeshComponent"] = [](const ECS::Entity &entity, nlohmann::json &data,
                                        Core::ResourceManager &resources) {
        if (entity.HasComponent<ECS::Components::MeshComponent>()) {
            if (const auto *mc = entity.GetComponent<ECS::Components::MeshComponent>(); mc->mesh) {
                if (std::string id = resources.GetKey<Rendering::Mesh>(mc->mesh); !id.empty()) {
                    data["MeshComponent"]["mesh_id"] = id;
                }
            }
        }
    };

    // MATERIAL COMPONENT
    s_Serializers["MaterialComponent"] = [](const ECS::Entity &entity, nlohmann::json &data,
                                            Core::ResourceManager &resources) {
        if (entity.HasComponent<ECS::Components::MaterialComponent>()) {
            if (const auto *mat = entity.GetComponent<ECS::Components::MaterialComponent>(); mat->material) {
                if (std::string id = resources.GetKey<Rendering::Material>(mat->material); !id.empty()) {
                    data["MaterialComponent"]["material_id"] = id;
                }
            }
        }
    };

    s_Serializers["BillboardTagComponent"] = [](const ECS::Entity &entity, nlohmann::json &data,
                                                Core::ResourceManager &) {
        if (entity.HasComponent<ECS::Components::BillboardTagComponent>()) {
            data["BillboardTagComponent"] = nlohmann::json::object();
        }
    };

    // DIRECTIONAL TEXTURE COMPONENT
    s_Serializers["DirectionalTextureComponent"] = [
            ](const ECS::Entity &entity, nlohmann::json &data, Core::ResourceManager &resources) {
                if (entity.HasComponent<ECS::Components::DirectionalTextureComponent>()) {
                    auto *dt = entity.GetComponent<ECS::Components::DirectionalTextureComponent>();
                    data["DirectionalTextureComponent"]["index"] = dt->index;

                    auto &texArray = data["DirectionalTextureComponent"]["textures"];
                    for (int i = 0; i < 6; ++i) {
                        if (dt->textures[i]) {
                            texArray.push_back(resources.GetKey<Rendering::Texture>(dt->textures[i]));
                        } else {
                            texArray.push_back("");
                        }
                    }
                }
            };

    // POINT LIGHT COMPONENT
    s_Serializers["PointLightComponent"] = [
            ](const ECS::Entity &entity, nlohmann::json &data, Core::ResourceManager &) {
                if (entity.HasComponent<ECS::Components::PointLightComponent>()) {
                    const auto *plc = entity.GetComponent<ECS::Components::PointLightComponent>();
                    data["PointLightComponent"]["color"] = {plc->color.x, plc->color.y, plc->color.z};
                    data["PointLightComponent"]["radius"] = plc->radius;
                    data["PointLightComponent"]["intensity"] = plc->intensity;
                }
            };

    // SCRIPT COMPONENT
    s_Serializers["ScriptComponent"] = [](const ECS::Entity &entity, nlohmann::json &data, Core::ResourceManager &) {
        if (entity.HasComponent<ECS::Components::ScriptComponent>()) {
            if (const auto *scriptComp = entity.GetComponent<ECS::Components::ScriptComponent>();
                scriptComp->scriptPaths.size() == 1) {
                // Single script
                data["ScriptComponent"]["scriptPath"] = scriptComp->scriptPaths[0];
            } else if (scriptComp->scriptPaths.size() > 1) {
                // Multiple scripts
                nlohmann::json scriptPathsArray = nlohmann::json::array();
                for (const auto &scriptPath: scriptComp->scriptPaths) {
                    scriptPathsArray.push_back(scriptPath);
                }
                data["ScriptComponent"]["scriptPaths"] = scriptPathsArray;
            }
        }
    };
}

void IO::EntityFactory::DeserializeEntity(ECS::Entity &entity, const nlohmann::json &entityData,
                                          Core::ResourceManager &resources) {
    if (!entityData.contains("components")) { return; }
    if (entityData.contains("name")) {
        entity.SetName(entityData["name"]);
    }
    for (const auto &[compName, compData]: entityData["components"].items()) {
        if (auto it = s_Deserializers.find(compName); it != s_Deserializers.end()) {
            it->second(entity, compData, resources);
        } else {
            std::cerr << "EntityFactory: No deserializer found for component '" << compName << "'\n";
        }
    }
}

void IO::EntityFactory::SerializeEntity(
    ECS::Entity &entity, nlohmann::json
    &outEntityData,
    Core::ResourceManager &resources) {
    if (!entity.GetName().empty()) {
        outEntityData["name"] = entity.GetName();
    }
    nlohmann::json componentsData;
    for (const auto &serializeFunc: s_Serializers | std::views::values) {
        serializeFunc(entity, componentsData, resources);
    }
    if (!componentsData.empty()) {
        outEntityData["components"] = componentsData;
    }
}
