#pragma once

#include "Core/ResourceManager.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/Types.h"
#include "ECS/Components/RelationshipComponent.h"
#include "IO/Loaders/EntityFactory.h"
#include "nlohmann/json.hpp"
#include <functional>
#include <unordered_map>
#include <vector>

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

    // entity tree
    inline nlohmann::json CopyEntityTreeToJson(const EntityID &entity, Registry *reg) {
        nlohmann::json out;
        if (!reg || !reg->IsValid(entity))
            return out;

        nlohmann::json entities = nlohmann::json::array();
        std::unordered_map<EntityID, size_t> idToIndex;

        std::function<void(EntityID)> serializeSubtree = [&](const EntityID id) {
            if (!reg->IsValid(id))
                return;

            const size_t index = entities.size();
            idToIndex[id] = index;

            Entity ent(id, reg);
            nlohmann::json entityJson;
            IO::EntityFactory::SerializeEntity(ent, entityJson, Core::ResourceManager::GetInstance());

            if (const auto *rel = reg->GetComponent<Components::RelationshipComponent>(id); rel && rel->parent != INVALID_ENTITY_ID) {
                if (const auto it = idToIndex.find(rel->parent); it != idToIndex.end())
                    entityJson["parent"] = it->second;
            }

            entities.push_back(std::move(entityJson));

            // snapshot
            const auto *rel = reg->GetComponent<Components::RelationshipComponent>(id);
            const std::vector<EntityID> children = rel ? rel->children : std::vector<EntityID>{};
            for (const EntityID child : children)
                serializeSubtree(child);
        };
        serializeSubtree(entity);

        out["entities"] = std::move(entities);
        out["rootIndex"] = 0;
        return out;
    }

    // instantiates what is produced by CopyEntityTreeToJson
    // returns the new root entity.
    // if parentOverride is valid, the root is re-parented
    inline EntityID InsertEntityTreeJson(const nlohmann::json &data, Registry *reg, const EntityID parentOverride = INVALID_ENTITY_ID) {
        if (!reg || !data.contains("entities") || !data["entities"].is_array() || data["entities"].empty())
            return INVALID_ENTITY_ID;

        const auto &entities = data["entities"];
        std::vector<EntityID> newIds(entities.size(), INVALID_ENTITY_ID);

        for (size_t i = 0; i < entities.size(); ++i) {
            const EntityID newId = reg->CreateEntity();
            Entity newEnt(newId, reg);
            IO::EntityFactory::DeserializeEntity(newEnt, entities[i], Core::ResourceManager::GetInstance());
            newIds[i] = newId;
        }

        const size_t rootIndex = data.value("rootIndex", 0);
        if (rootIndex >= newIds.size())
            return newIds[0];

        for (size_t i = 0; i < entities.size(); ++i) {
            if (i == rootIndex || !entities[i].contains("parent"))
                continue;
            const size_t parentIndex = entities[i]["parent"].get<size_t>();
            if (parentIndex < newIds.size() && newIds[i] != INVALID_ENTITY_ID && newIds[parentIndex] != INVALID_ENTITY_ID)
                reg->SetParentDirect(newIds[i], newIds[parentIndex]);
        }

        const EntityID rootId = newIds[rootIndex];
        if (rootId == INVALID_ENTITY_ID)
            return INVALID_ENTITY_ID;

        if (parentOverride != INVALID_ENTITY_ID && reg->IsValid(parentOverride))
            reg->Reparent(rootId, parentOverride);

        return rootId;
    }

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
