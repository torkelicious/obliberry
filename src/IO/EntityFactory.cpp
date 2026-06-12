#include "EntityFactory.h"
#include <iostream>

#include "ECS/ECS.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/PlayerInputComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/BillboardComponent.h"
#include "Renderer/Mesh.h"
#include "ECS/Components/PointLightComponent.h"

std::unordered_map<std::string, ComponentDeserializer> EntityFactory::s_Deserializers;
std::unordered_map<std::string, ComponentSerializer> EntityFactory::s_Serializers;

const std::unordered_map<std::string, ComponentDeserializer> &EntityFactory::GetDeserializers() {
    return s_Deserializers;
}

const std::unordered_map<std::string, ComponentSerializer> &EntityFactory::GetSerializers() {
    return s_Serializers;
}

void EntityFactory::RegisterDeserializers() {
    if (!s_Deserializers.empty()) return;

    // TRANSFORM COMPONENT
    s_Deserializers["TransformComponent"] = [](Entity &entity, const nlohmann::json &data, ResourceManager &resources) {
        TransformComponent tc;
        if (data.contains("position")) {
            tc.transform.SetPosition({data["position"][0], data["position"][1], data["position"][2]});
        }
        if (data.contains("scale")) {
            tc.transform.SetScale({data["scale"][0], data["scale"][1], data["scale"][2]});
        }
        entity.AddComponent<TransformComponent>(tc);
    };

    // MOVEMENT COMPONENT
    s_Deserializers["MovementComponent"] = [](Entity &entity, const nlohmann::json &data, ResourceManager &resources) {
        MovementComponent mc;
        if (data.contains("timePerStep")) {
            mc.timePerStep = data["timePerStep"].get<float>();
        }
        entity.AddComponent<MovementComponent>(mc);
    };

    // PLAYER INPUT COMPONENT
    s_Deserializers["PlayerInputComponent"] = [
            ](Entity &entity, const nlohmann::json &data, ResourceManager &resources) {
                PlayerInputComponent input;
                if (data.contains("LeftClick")) input.LeftClick = data["LeftClick"].get<int>();
                if (data.contains("RightClick")) input.RightClick = data["RightClick"].get<int>();
                if (data.contains("Up")) input.Up = data["Up"].get<int>();
                if (data.contains("Down")) input.Down = data["Down"].get<int>();
                if (data.contains("Left")) input.Left = data["Left"].get<int>();
                if (data.contains("Right")) input.Right = data["Right"].get<int>();
                if (data.contains("Quit")) input.Quit = data["Quit"].get<int>();
                entity.AddComponent<PlayerInputComponent>(input);
            };

    // MESH COMPONENT
    s_Deserializers["MeshComponent"] = [](Entity &entity, const nlohmann::json &data, ResourceManager &resources) {
        MeshComponent mc;
        if (data.contains("mesh_id")) {
            const std::string meshId = data["mesh_id"].get<std::string>();
            mc.mesh = resources.Get<Mesh>(meshId);
            if (!mc.mesh) std::cerr << "EntityFactory: Failed to find Mesh ID '" << meshId << "'\n";
        }
        entity.AddComponent<MeshComponent>(mc);
    };

    // MATERIAL COMPONENT
    s_Deserializers["MaterialComponent"] = [](Entity &entity, const nlohmann::json &data, ResourceManager &resources) {
        MaterialComponent matComp;
        if (data.contains("material_id")) {
            const std::string matID = data["material_id"].get<std::string>();
            matComp.material = resources.Get<Material>(matID);
            if (!matComp.material) std::cerr << "EntityFactory: Failed to find Material ID '" << matID << "'\n";
        }
        entity.AddComponent<MaterialComponent>(matComp);
    };

    s_Deserializers["BillboardComponent"] = [](Entity &entity, const nlohmann::json &, ResourceManager &) {
        entity.AddComponent<BillboardComponent>();
    };

    // DIRECTIONAL TEXTURE COMPONENT
    s_Deserializers["DirectionalTextureComponent"] = [](Entity &entity, const nlohmann::json &data,
                                                        ResourceManager &resources) {
        DirectionalTextureComponent dirTex;
        if (data.contains("textures") && data["textures"].is_array()) {
            auto &texArray = data["textures"];
            for (size_t i = 0; i < 6 && i < texArray.size(); ++i) {
                auto texID = texArray[i].get<std::string>();
                dirTex.textures[i] = resources.Get<Texture>(texID);
            }
        }
        if (data.contains("index")) {
            dirTex.index = data["index"].get<int>();
        }
        entity.AddComponent<DirectionalTextureComponent>(dirTex);
    };

    // POINT LIGHT COMPONENT
    s_Deserializers["PointLightComponent"] = [](Entity &entity, const nlohmann::json &data, ResourceManager &) {
        PointLightComponent plc;
        if (data.contains("color")) {
            plc.color = {data["color"][0], data["color"][1], data["color"][2]};
        }
        if (data.contains("radius")) {
            plc.radius = data["radius"].get<float>();
        }
        if (data.contains("intensity")) {
            plc.intensity = data["intensity"].get<float>();
        }
        entity.AddComponent<PointLightComponent>(plc);
    };
}

void EntityFactory::RegisterSerializers() {
    if (!s_Serializers.empty()) return;

    // TRANSFORM COMPONENT
    s_Serializers["TransformComponent"] = [](const Entity &entity, nlohmann::json &data, ResourceManager &resources) {
        if (entity.HasComponent<TransformComponent>()) {
            const auto *tc = entity.GetComponent<TransformComponent>();
            auto pos = tc->transform.GetPosition();
            auto scale = tc->transform.GetScale();
            data["TransformComponent"]["position"] = {pos.x, pos.y, pos.z};
            data["TransformComponent"]["scale"] = {scale.x, scale.y, scale.z};
        }
    };

    // MOVEMENT COMPONENT
    s_Serializers["MovementComponent"] = [](const Entity &entity, nlohmann::json &data, ResourceManager &resources) {
        if (entity.HasComponent<MovementComponent>()) {
            auto *mc = entity.GetComponent<MovementComponent>();
            data["MovementComponent"]["timePerStep"] = mc->timePerStep;
            // state data is ignored
        }
    };

    // PLAYER INPUT COMPONENT
    s_Serializers["PlayerInputComponent"] = [](const Entity &entity, nlohmann::json &data, ResourceManager &resources) {
        if (entity.HasComponent<PlayerInputComponent>()) {
            auto *pic = entity.GetComponent<PlayerInputComponent>();
            data["PlayerInputComponent"]["LeftClick"] = pic->LeftClick;
            data["PlayerInputComponent"]["RightClick"] = pic->RightClick;
            data["PlayerInputComponent"]["Up"] = pic->Up;
            data["PlayerInputComponent"]["Down"] = pic->Down;
            data["PlayerInputComponent"]["Left"] = pic->Left;
            data["PlayerInputComponent"]["Right"] = pic->Right;
            data["PlayerInputComponent"]["Quit"] = pic->Quit;
        }
    };

    // MESH COMPONENT
    s_Serializers["MeshComponent"] = [](const Entity &entity, nlohmann::json &data, ResourceManager &resources) {
        if (entity.HasComponent<MeshComponent>()) {
            if (const auto *mc = entity.GetComponent<MeshComponent>(); mc->mesh) {
                if (std::string id = resources.GetKey<Mesh>(mc->mesh); !id.empty()) {
                    data["MeshComponent"]["mesh_id"] = id;
                }
            }
        }
    };

    // MATERIAL COMPONENT
    s_Serializers["MaterialComponent"] = [](const Entity &entity, nlohmann::json &data, ResourceManager &resources) {
        if (entity.HasComponent<MaterialComponent>()) {
            if (const auto *mat = entity.GetComponent<MaterialComponent>(); mat->material) {
                if (std::string id = resources.GetKey<Material>(mat->material); !id.empty()) {
                    data["MaterialComponent"]["material_id"] = id;
                }
            }
        }
    };

    s_Serializers["BillboardComponent"] = [](const Entity &entity, nlohmann::json &data, ResourceManager &) {
        if (entity.HasComponent<BillboardComponent>()) {
            data["BillboardComponent"] = nlohmann::json::object();
        }
    };

    // DIRECTIONAL TEXTURE COMPONENT
    s_Serializers["DirectionalTextureComponent"] = [
            ](const Entity &entity, nlohmann::json &data, ResourceManager &resources) {
                if (entity.HasComponent<DirectionalTextureComponent>()) {
                    auto *dt = entity.GetComponent<DirectionalTextureComponent>();
                    data["DirectionalTextureComponent"]["index"] = dt->index;

                    auto &texArray = data["DirectionalTextureComponent"]["textures"];
                    for (int i = 0; i < 6; ++i) {
                        if (dt->textures[i]) {
                            texArray.push_back(resources.GetKey<Texture>(dt->textures[i]));
                        } else {
                            texArray.push_back("");
                        }
                    }
                }
            };

    // POINT LIGHT COMPONENT
    s_Serializers["PointLightComponent"] = [](const Entity &entity, nlohmann::json &data, ResourceManager &) {
        if (entity.HasComponent<PointLightComponent>()) {
            const auto *plc = entity.GetComponent<PointLightComponent>();
            data["PointLightComponent"]["color"] = {plc->color.x, plc->color.y, plc->color.z};
            data["PointLightComponent"]["radius"] = plc->radius;
            data["PointLightComponent"]["intensity"] = plc->intensity;
        }
    };
}

void EntityFactory::DeserializeEntity(Entity &entity, const nlohmann::json &entityData, ResourceManager &resources) {
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

void EntityFactory::SerializeEntity(
    Entity &entity, nlohmann::json
    &outEntityData,
    ResourceManager &resources) {
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
