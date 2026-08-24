#pragma once

#include "ECS/Components/RelationshipComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Registry.h"
#include <queue>

namespace ECS::Systems::HierarchySystem {

    inline void Propagate(Registry &registry) {
        std::queue<EntityID> queue;

        // cache the pools
        auto *tcPool = registry.GetPool<Components::TransformComponent>();
        auto *relPool = registry.GetPool<Components::RelationshipComponent>();

        for (const EntityID id : tcPool->GetDenseEntities()) {
            auto *tc = tcPool->Get(id);

            // if it has no relationship / no parent. it is a root node.
            if (const auto *rel = relPool->Get(id); !rel || rel->parent == INVALID_ENTITY_ID) {
                const auto &local = tc->transform;
                tc->worldTransform.SetPosition(local.GetPosition());
                tc->worldTransform.SetRotation(local.GetRotation());
                tc->worldTransform.SetScale(local.GetScale());
                tc->worldTransform.SetCustomMatrix(local.GetMatrix());

                // queue if it has children
                if (rel && !rel->children.empty()) {
                    queue.push(id);
                }
            }
        }

        while (!queue.empty()) {
            const EntityID parentId = queue.front();
            queue.pop();

            const auto *parentTc = tcPool->Get(parentId);
            const auto *parentRel = relPool->Get(parentId);

            if (!parentTc || !parentRel)
                continue;

            const glm::mat4 &parentWorldMatrix = parentTc->worldTransform.GetMatrix();

            for (const EntityID childId : parentRel->children) {
                auto *childTc = tcPool->Get(childId);
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

                // queue this child if it also has children of its own
                if (const auto *childRel = relPool->Get(childId); childRel && !childRel->children.empty()) {
                    queue.push(childId);
                }
            }
        }
    }

} // namespace ECS::Systems::HierarchySystem
