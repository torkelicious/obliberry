#include "Core/ResourceManager.h"
#include "ECS/Components/RelationshipComponent.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/Types.h"
#include "IO/Loaders/EntityFactory.h"
#include "nlohmann/json_fwd.hpp"

namespace ECS::Utils {

    //
    // I want to make a copy / paste functionallity
    // type shi where you can select multiples evem idk
    // coming soon????
    //


    inline nlohmann::json CopyEntityToJson(const EntityID &entity, Registry *reg) {
        nlohmann::json data;
        auto ent = Entity(entity, reg);
        IO::EntityFactory::SerializeEntity(ent, data, Core::ResourceManager::GetInstance());
        return data;
    }

    inline void InsertEntityJson(nlohmann::json &data, Registry *reg) {
        if (data.contains("name")) {
            std::string name = data["name"];
            data["name"] = name + " (copy)";
        }
        auto newId = reg->CreateEntity();
        Entity newEnt = Entity(newId, reg);
        IO::EntityFactory::DeserializeEntity(newEnt, data, Core::ResourceManager::GetInstance());
    }

    inline void PasteEntity(const EntityID &entityId, Registry *reg) {
        auto _Entity = Entity(entityId, reg);
        nlohmann::json data;
        IO::EntityFactory::SerializeEntity(_Entity, data, Core::ResourceManager::GetInstance());
        InsertEntityJson(data, reg);
    }

    inline bool IsAncestorOf(ECS::Registry &registry, const ECS::EntityID entity, const ECS::EntityID candidate) {
        const auto *rel = registry.GetComponent<ECS::Components::RelationshipComponent>(entity);
        while (rel && rel->parent != ECS::INVALID_ENTITY_ID) {
            if (rel->parent == candidate)
                return true;
            rel = registry.GetComponent<ECS::Components::RelationshipComponent>(rel->parent);
        }
        return false;
    }


} // namespace ECS::Utils
