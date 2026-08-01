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

                const auto worldPos = glm::vec3(parentWorldMatrix * glm::vec4(childTc->transform.GetPosition(), 1.0f));

                const glm::vec3 worldScale = parentScale * childTc->transform.GetScale();

                const glm::vec3 worldRot = parentRot + childTc->transform.GetRotation();

                childTc->worldTransform.SetPosition(worldPos);
                childTc->worldTransform.SetRotation(worldRot);
                childTc->worldTransform.SetScale(worldScale);
                childTc->worldTransform.SetCustomMatrix(parentWorldMatrix * localMatrix);

                queue.push(childId);
            }
        }
    }

} // namespace ECS::Systems::HierarchySystem
