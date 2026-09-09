#pragma once

#include "Core/ResourceManager.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/Types.h"
#include "ECS/Components/RelationshipComponent.h"
#include "IO/Loaders/EntityFactory.h"
#include "nlohmann/json.hpp"

namespace ECS::Utils {

    // serializes a single entity  (comp only)
    inline nlohmann::json CopyEntityToJson(const EntityID &entity, Registry *reg) {
        nlohmann::json data;
        auto ent = Entity(entity, reg);
        IO::EntityFactory::SerializeEntity(ent, data, Core::ResourceManager::GetInstance());
        return data;
    }

    // creates a new entity from serialized data
    inline EntityID InsertEntityJson(nlohmann::json data, Registry *reg) {
        if (!reg || data.empty())
            return INVALID_ENTITY_ID;

        if (data.contains("name")) {
            const std::string name = data["name"];
            data["name"] = name + " (copy)";
        }

        const EntityID newId = reg->CreateEntity();
        Entity newEnt(newId, reg);
        IO::EntityFactory::DeserializeEntity(newEnt, data, Core::ResourceManager::GetInstance());
        return newId;
    }

    // duplicates an entity in place
    inline EntityID PasteEntity(const EntityID &entityId, Registry *reg) { return InsertEntityJson(CopyEntityToJson(entityId, reg), reg); }

    // True when candidate is an ancestor of entity
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
