#pragma once

#include "ECS/Components/RelationshipComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Registry.h"
#include <queue>

namespace ECS::Systems::HierarchySystem {

    inline void Propagate(Registry &registry) {
        registry.ForEach<Components::TransformComponent>([&](Entity, Components::TransformComponent *tc) {
            const auto &local = tc->transform;
            tc->worldTransform.SetPosition(local.GetPosition());
            tc->worldTransform.SetRotation(local.GetRotation());
            tc->worldTransform.SetScale(local.GetScale());
            tc->worldTransform.SetCustomMatrix(local.GetMatrix());
        });

        std::queue<EntityID> queue;

        registry.ForEach<Components::RelationshipComponent>([&](const Entity entity, const Components::RelationshipComponent *rel) {
            if (rel->parent == INVALID_ENTITY_ID) {
                queue.push(static_cast<EntityID>(entity));
            }
        });

        while (!queue.empty()) {
            const EntityID parentId = queue.front();
            queue.pop();

            const auto *parentTc = registry.GetComponent<Components::TransformComponent>(parentId);
            const auto *parentRel = registry.GetComponent<Components::RelationshipComponent>(parentId);
            if (!parentTc || !parentRel)
                continue;

            const glm::mat4 &parentWorldMatrix = parentTc->worldTransform.GetMatrix();
            const glm::vec3 &parentScale = parentTc->worldTransform.GetScale();
            const glm::vec3 &parentRot = parentTc->worldTransform.GetRotation();

            for (const EntityID childId : parentRel->children) {
                auto *childTc = registry.GetComponent<Components::TransformComponent>(childId);
                if (!childTc)
                    continue;

                const glm::mat4 localMatrix = childTc->transform.GetMatrix();
                const glm::mat4 worldMatrix = parentWorldMatrix * localMatrix;

                glm::vec3 position, scale, skew;
                glm::quat rotation;
                glm::vec4 perspective;
                glm::decompose(worldMatrix, scale, rotation, position, skew, perspective);

                childTc->worldTransform.SetPosition(position);
                childTc->worldTransform.SetRotation(glm::eulerAngles(rotation));
                childTc->worldTransform.SetScale(scale);
                childTc->worldTransform.SetCustomMatrix(worldMatrix);

                queue.push(childId);
            }
        }
    }

} // namespace ECS::Systems::HierarchySystem
