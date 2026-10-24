#pragma once

#include "ECS/Components/RelationshipComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Registry.h"
#include <cmath>
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
            if (const auto *rel = relPool->Get(id); !rel || rel->parent == INVALID_ENTITY_ID || !registry.IsValid(rel->parent)) {
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

        for (const EntityID id : relPool->GetDenseEntities()) {
            if (tcPool->Get(id)) {
                continue;
            }

            if (const auto *rel = relPool->Get(id); rel && rel->parent == INVALID_ENTITY_ID && !rel->children.empty()) {
                queue.push(id);
            }
        }

        while (!queue.empty()) {
            const EntityID parentId = queue.front();
            queue.pop();

            const auto *parentRel = relPool->Get(parentId);
            if (!parentRel)
                continue;

            const auto *parentTc = tcPool->Get(parentId);
            const glm::mat4 parentWorldMatrix = parentTc ? parentTc->worldTransform.GetMatrix() : glm::mat4(1.0f);

            for (const EntityID childId : parentRel->children) {
                if (const auto *childRel = relPool->Get(childId); childRel && !childRel->children.empty()) {
                    queue.push(childId);
                }

                auto *childTc = tcPool->Get(childId);
                if (!childTc)
                    continue;

                const glm::mat4 localMatrix = childTc->transform.GetMatrix();
                const glm::mat4 worldMatrix = parentWorldMatrix * localMatrix;

                if (!Math::GLMUtils::MatIsFinite(worldMatrix)) {
                    continue;
                }

                glm::vec3 position, scale, skew;
                glm::quat rotation;
                glm::vec4 perspective;
                const bool decomposed = glm::decompose(worldMatrix, scale, rotation, position, skew, perspective) && Math::GLMUtils::VecIsFinite(position) && Math::GLMUtils::VecIsFinite(scale) &&
                                        std::isfinite(rotation.x) && std::isfinite(rotation.y) && std::isfinite(rotation.z) && std::isfinite(rotation.w);

                if (decomposed) {
                    childTc->worldTransform.SetPosition(position);
                    glm::vec3 rotationAngles;
                    glm::extractEulerAngleXYZ(glm::mat4_cast(rotation), rotationAngles.x, rotationAngles.y, rotationAngles.z);
                    childTc->worldTransform.SetRotation(rotationAngles);
                    childTc->worldTransform.SetScale(scale);
                } else {
                    // Finite singular matrices are still usable.
                    childTc->worldTransform.SetPosition(Math::GLMUtils::MatrixPosition(worldMatrix));
                    childTc->worldTransform.SetRotation(glm::vec3(0.0f));
                    childTc->worldTransform.SetScale(Math::GLMUtils::MatrixScale(worldMatrix));
                }

                childTc->worldTransform.SetCustomMatrix(worldMatrix);
            }
        }
    }

} // namespace ECS::Systems::HierarchySystem
