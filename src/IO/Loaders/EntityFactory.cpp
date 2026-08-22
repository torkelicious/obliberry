#include "EntityFactory.h"
#include "Logger/LoggerService.h"

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
#include "ECS/Components/PrefabSourceComponent.h"
#include "ECS/Components/ParticleEmitterComponent.h"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "EntityFactory"


// note: Prefabs not serialized / deserialized (here)

std::unordered_map<std::string, ComponentDeserializer> IO::EntityFactory::s_Deserializers;
std::unordered_map<std::string, ComponentSerializer> IO::EntityFactory::s_Serializers;

const std::unordered_map<std::string, ComponentDeserializer> &IO::EntityFactory::GetDeserializers() { return s_Deserializers; }

const std::unordered_map<std::string, ComponentSerializer> &IO::EntityFactory::GetSerializers() { return s_Serializers; }

void IO::EntityFactory::RegisterDeserializers() {
    if (!s_Deserializers.empty())
        return;

    // TRANSFORM COMPONENT
    s_Deserializers["TransformComponent"] = [](ECS::Entity &entity, const nlohmann::json &data, Core::ResourceManager & /*resources*/) {
        ECS::Components::TransformComponent tc;
        if (data.contains("position")) {
            tc.transform.SetPosition({data["position"][0], data["position"][1], data["position"][2]});
        }
        if (data.contains("rotation")) {
            tc.transform.SetRotation({data["rotation"][0], data["rotation"][1], data["rotation"][2]});
        }
        if (data.contains("scale")) {
            tc.transform.SetScale({data["scale"][0], data["scale"][1], data["scale"][2]});
        }
        entity.AddComponent<ECS::Components::TransformComponent>(tc);
    };

    // MOVEMENT COMPONENT
    s_Deserializers["MovementComponent"] = [](ECS::Entity &entity, const nlohmann::json &data, Core::ResourceManager & /*resources*/) {
        ECS::Components::MovementComponent mc;
        if (data.contains("timePerStep")) {
            mc.timePerStep = data["timePerStep"].get<float>();
        }
        if (data.contains("autoMove")) {
            mc.autoMove = data["autoMove"].get<bool>();
        }
        entity.AddComponent<ECS::Components::MovementComponent>(mc);
    };

    // MESH COMPONENT
    s_Deserializers["MeshComponent"] = [](ECS::Entity &entity, const nlohmann::json &data, Core::ResourceManager &resources) {
        ECS::Components::MeshComponent mc;
        if (data.contains("mesh_id")) {
            const std::string meshId = data["mesh_id"].get<std::string>();
            mc.mesh = resources.Get<Rendering::Mesh>(meshId);
            if (!mc.mesh)
                LOG_ERROR(LOG_WHO, "Failed to find Mesh ID '" + meshId + "'");
        }
        entity.AddComponent<ECS::Components::MeshComponent>(mc);
    };

    // MATERIAL COMPONENT
    s_Deserializers["MaterialComponent"] = [](ECS::Entity &entity, const nlohmann::json &data, Core::ResourceManager &resources) {
        ECS::Components::MaterialComponent matComp;
        if (data.contains("material_id")) {
            const std::string matID = data["material_id"].get<std::string>();
            matComp.material = resources.Get<Rendering::Material>(matID);
            if (!matComp.material)
                LOG_ERROR(LOG_WHO, "Failed to find Material ID '" + matID + "'");
        }
        entity.AddComponent<ECS::Components::MaterialComponent>(matComp);
    };

    s_Deserializers["BillboardTagComponent"] = [](ECS::Entity &entity, const nlohmann::json &, Core::ResourceManager &) { entity.AddComponent<ECS::Components::BillboardTagComponent>(); };

    // DIRECTIONAL TEXTURE COMPONENT
    s_Deserializers["DirectionalTextureComponent"] = [](ECS::Entity &entity, const nlohmann::json &data, Core::ResourceManager &resources) {
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
    s_Deserializers["PointLightComponent"] = [](ECS::Entity &entity, const nlohmann::json &data, Core::ResourceManager &) {
        ECS::Components::PointLightComponent plc;
        if (data.contains("color")) {
            plc.SetColor({data["color"][0], data["color"][1], data["color"][2]});
        }
        if (data.contains("radius")) {
            plc.SetRadius(data["radius"].get<float>());
        }
        if (data.contains("intensity")) {
            plc.SetIntensity(data["intensity"].get<float>());
        }
        entity.AddComponent<ECS::Components::PointLightComponent>(plc);
    };

    // SCRIPT COMPONENT
    s_Deserializers["ScriptComponent"] = [](ECS::Entity &entity, const nlohmann::json &data, Core::ResourceManager & /*resources*/) {
        auto &scriptComp = entity.AddComponent<ECS::Components::ScriptComponent>();

        std::vector<std::string> paths;
        if (data.contains("scriptPath")) {
            // Single script
            paths.push_back(data["scriptPath"].get<std::string>());
        } else if (data.contains("scriptPaths") && data["scriptPaths"].is_array()) {
            // Multiple scripts
            for (const auto &scriptPath : data["scriptPaths"]) {
                paths.push_back(scriptPath.get<std::string>());
            }
        }

        scriptComp.slots.resize(paths.size());
        for (size_t i = 0; i < paths.size(); ++i) {
            scriptComp.slots[i].scriptPath = std::move(paths[i]);
        }
    };

    // PARTICLE EMITTER COMPONENT
    s_Deserializers["ParticleEmitterComponent"] = [](ECS::Entity &entity, const nlohmann::json &data, Core::ResourceManager &resources) {
        ECS::Components::ParticleEmitterComponent ec;
        if (data.contains("maxParticles"))
            ec.maxParticles = data["maxParticles"].get<int>();
        if (data.contains("emitRate"))
            ec.emitRate = data["emitRate"].get<float>();
        if (data.contains("lifetimeMin"))
            ec.lifetimeMin = data["lifetimeMin"].get<float>();
        if (data.contains("lifetimeMax"))
            ec.lifetimeMax = data["lifetimeMax"].get<float>();
        if (data.contains("velocityMin"))
            ec.velocityMin = {data["velocityMin"][0], data["velocityMin"][1], data["velocityMin"][2]};
        if (data.contains("velocityMax"))
            ec.velocityMax = {data["velocityMax"][0], data["velocityMax"][1], data["velocityMax"][2]};
        if (data.contains("gravity"))
            ec.gravity = {data["gravity"][0], data["gravity"][1], data["gravity"][2]};
        if (data.contains("sizeStartMin"))
            ec.sizeStartMin = data["sizeStartMin"].get<float>();
        else if (data.contains("sizeStart"))
            ec.sizeStartMin = ec.sizeStartMax = data["sizeStart"].get<float>();
        if (data.contains("sizeStartMax"))
            ec.sizeStartMax = data["sizeStartMax"].get<float>();
        if (data.contains("sizeEndMin"))
            ec.sizeEndMin = data["sizeEndMin"].get<float>();
        else if (data.contains("sizeEnd"))
            ec.sizeEndMin = ec.sizeEndMax = data["sizeEnd"].get<float>();
        if (data.contains("sizeEndMax"))
            ec.sizeEndMax = data["sizeEndMax"].get<float>();
        if (data.contains("rotationSpeedMin"))
            ec.rotationSpeedMin = data["rotationSpeedMin"].get<float>();
        if (data.contains("rotationSpeedMax"))
            ec.rotationSpeedMax = data["rotationSpeedMax"].get<float>();
        if (data.contains("colorStart"))
            ec.colorStart = {data["colorStart"][0], data["colorStart"][1], data["colorStart"][2], data["colorStart"][3]};
        if (data.contains("colorEnd"))
            ec.colorEnd = {data["colorEnd"][0], data["colorEnd"][1], data["colorEnd"][2], data["colorEnd"][3]};
        if (data.contains("isBillboard"))
            ec.isBillboard = data["isBillboard"].get<bool>();
        if (data.contains("blendMode")) {
            const int bm = data["blendMode"].get<int>();
            ec.blendMode = bm == 1 ? ECS::Components::ParticleBlendMode::Additive : ECS::Components::ParticleBlendMode::Alpha;
        }
        if (data.contains("renderOrder"))
            ec.renderOrder = data["renderOrder"].get<int>();
        if (data.contains("shape"))
            ec.shape = data["shape"].get<int>();
        if (data.contains("material_id")) {
            const std::string matID = data["material_id"].get<std::string>();
            ec.material = resources.Get<Rendering::Material>(matID);
        }
        entity.AddComponent<ECS::Components::ParticleEmitterComponent>(ec);
    };
}

void IO::EntityFactory::RegisterSerializers() {
    if (!s_Serializers.empty())
        return;

    // TRANSFORM COMPONENT
    s_Serializers["TransformComponent"] = [](const ECS::Entity &entity, nlohmann::json &data, Core::ResourceManager & /*resources*/) {
        if (entity.HasComponent<ECS::Components::TransformComponent>()) {
            const auto *tc = entity.GetComponent<ECS::Components::TransformComponent>();
            auto pos = tc->transform.GetPosition();
            auto rot = tc->transform.GetRotation();
            auto scale = tc->transform.GetScale();
            data["TransformComponent"]["position"] = {pos.x, pos.y, pos.z};
            data["TransformComponent"]["rotation"] = {rot.x, rot.y, rot.z};
            data["TransformComponent"]["scale"] = {scale.x, scale.y, scale.z};
        }
    };

    // MOVEMENT COMPONENT
    s_Serializers["MovementComponent"] = [](const ECS::Entity &entity, nlohmann::json &data, Core::ResourceManager & /*resources*/) {
        if (entity.HasComponent<ECS::Components::MovementComponent>()) {
            auto *mc = entity.GetComponent<ECS::Components::MovementComponent>();
            data["MovementComponent"]["timePerStep"] = mc->timePerStep;
            data["MovementComponent"]["autoMove"] = mc->autoMove;
            // state data is ignored
        }
    };

    // MESH COMPONENT
    s_Serializers["MeshComponent"] = [](const ECS::Entity &entity, nlohmann::json &data, Core::ResourceManager &resources) {
        if (entity.HasComponent<ECS::Components::MeshComponent>()) {
            if (const auto *mc = entity.GetComponent<ECS::Components::MeshComponent>(); mc->mesh) {
                if (std::string id = resources.GetKey<Rendering::Mesh>(mc->mesh); !id.empty()) {
                    data["MeshComponent"]["mesh_id"] = id;
                }
            }
        }
    };

    // MATERIAL COMPONENT
    s_Serializers["MaterialComponent"] = [](const ECS::Entity &entity, nlohmann::json &data, Core::ResourceManager &resources) {
        if (entity.HasComponent<ECS::Components::MaterialComponent>()) {
            if (const auto *mat = entity.GetComponent<ECS::Components::MaterialComponent>(); mat->material) {
                if (std::string id = resources.GetKey<Rendering::Material>(mat->material); !id.empty()) {
                    data["MaterialComponent"]["material_id"] = id;
                }
            }
        }
    };

    s_Serializers["BillboardTagComponent"] = [](const ECS::Entity &entity, nlohmann::json &data, Core::ResourceManager &) {
        if (entity.HasComponent<ECS::Components::BillboardTagComponent>()) {
            data["BillboardTagComponent"] = nlohmann::json::object();
        }
    };

    // DIRECTIONAL TEXTURE COMPONENT
    s_Serializers["DirectionalTextureComponent"] = [](const ECS::Entity &entity, nlohmann::json &data, Core::ResourceManager &resources) {
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
    s_Serializers["PointLightComponent"] = [](const ECS::Entity &entity, nlohmann::json &data, Core::ResourceManager &) {
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
            if (const auto *scriptComp = entity.GetComponent<ECS::Components::ScriptComponent>(); scriptComp->slots.size() == 1) {
                // Single script
                data["ScriptComponent"]["scriptPath"] = scriptComp->slots[0].scriptPath;
            } else if (scriptComp->slots.size() > 1) {
                // Multiple scripts
                nlohmann::json scriptPathsArray = nlohmann::json::array();
                for (const auto &slot : scriptComp->slots) {
                    scriptPathsArray.push_back(slot.scriptPath);
                }
                data["ScriptComponent"]["scriptPaths"] = scriptPathsArray;
            }
        }
    };

    // PARTICLE EMITTER COMPONENT
    s_Serializers["ParticleEmitterComponent"] = [](const ECS::Entity &entity, nlohmann::json &data, Core::ResourceManager &resources) {
        if (entity.HasComponent<ECS::Components::ParticleEmitterComponent>()) {
            const auto *ec = entity.GetComponent<ECS::Components::ParticleEmitterComponent>();
            data["ParticleEmitterComponent"]["maxParticles"] = ec->maxParticles;
            data["ParticleEmitterComponent"]["emitRate"] = ec->emitRate;
            data["ParticleEmitterComponent"]["lifetimeMin"] = ec->lifetimeMin;
            data["ParticleEmitterComponent"]["lifetimeMax"] = ec->lifetimeMax;
            data["ParticleEmitterComponent"]["velocityMin"] = {ec->velocityMin.x, ec->velocityMin.y, ec->velocityMin.z};
            data["ParticleEmitterComponent"]["velocityMax"] = {ec->velocityMax.x, ec->velocityMax.y, ec->velocityMax.z};
            data["ParticleEmitterComponent"]["gravity"] = {ec->gravity.x, ec->gravity.y, ec->gravity.z};
            data["ParticleEmitterComponent"]["sizeStartMin"] = ec->sizeStartMin;
            data["ParticleEmitterComponent"]["sizeStartMax"] = ec->sizeStartMax;
            data["ParticleEmitterComponent"]["sizeEndMin"] = ec->sizeEndMin;
            data["ParticleEmitterComponent"]["sizeEndMax"] = ec->sizeEndMax;
            data["ParticleEmitterComponent"]["rotationSpeedMin"] = ec->rotationSpeedMin;
            data["ParticleEmitterComponent"]["rotationSpeedMax"] = ec->rotationSpeedMax;
            data["ParticleEmitterComponent"]["colorStart"] = {ec->colorStart.x, ec->colorStart.y, ec->colorStart.z, ec->colorStart.w};
            data["ParticleEmitterComponent"]["colorEnd"] = {ec->colorEnd.x, ec->colorEnd.y, ec->colorEnd.z, ec->colorEnd.w};
            data["ParticleEmitterComponent"]["isBillboard"] = ec->isBillboard;
            data["ParticleEmitterComponent"]["blendMode"] = ec->blendMode;
            data["ParticleEmitterComponent"]["renderOrder"] = ec->renderOrder;
            data["ParticleEmitterComponent"]["shape"] = ec->shape;
            if (ec->material) {
                if (std::string id = resources.GetKey<Rendering::Material>(ec->material); !id.empty()) {
                    data["ParticleEmitterComponent"]["material_id"] = id;
                }
            }
        }
    };
}

void IO::EntityFactory::DeserializeEntity(ECS::Entity &entity, const nlohmann::json &entityData, Core::ResourceManager &resources) {
    if (!entityData.contains("components")) {
        return;
    }
    if (entityData.contains("name")) {
        entity.SetName(entityData["name"]);
    }
    for (const auto &[compName, compData] : entityData["components"].items()) {
        if (auto it = s_Deserializers.find(compName); it != s_Deserializers.end()) {
            it->second(entity, compData, resources);
        } else {
            LOG_ERROR(LOG_WHO, "No deserializer found for component '" + compName + "'");
        }
    }
}

void IO::EntityFactory::SerializeEntity(ECS::Entity &entity, nlohmann::json &outEntityData, Core::ResourceManager &resources) {
    if (!entity.GetName().empty()) {
        outEntityData["name"] = entity.GetName();
    }
    nlohmann::json componentsData;
    for (const auto &serializeFunc : s_Serializers | std::views::values) {
        serializeFunc(entity, componentsData, resources);
    }
    if (!componentsData.empty()) {
        outEntityData["components"] = componentsData;
    }
}
#pragma pop_macro("LOG_WHO")
