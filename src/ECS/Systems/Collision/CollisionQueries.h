#pragma once

#include "ColliderGeometry.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Entity.h"
#include "ECS/Types.h"
#include "GJK.h"
#include "ECS/Registry.h"

namespace ECS::Collision {

    inline bool CanOccupy(Registry &registry, const EntityID entity, const glm::vec3 &targetWorldPos, const BillboardBasis &basis) {
        const auto *collider = registry.GetComponent<Components::ColliderComponent>(entity);
        const auto *transform = registry.GetComponent<Components::TransformComponent>(entity);

        if (!transform) {
            return false;
        }
        if (!collider || collider->isTrigger) {
            return true;
        }

        if (!Components::IsValidCollider(*collider)) {
            return false;
        }

        WorldCollider cand = BuildWorldCollider(entity, *collider, *transform, basis);

        // world space
        const glm::dvec3 displace = glm::dvec3(targetWorldPos) - glm::dvec3(transform->worldTransform.GetMatrix()[3]);

        cand.localToWorld[3] += glm::dvec4(displace, 0.0);

        const ColliderAABB candBounds = ComputeWorldAABB(cand);

        bool blocked = false;

        registry.ForEach<Components::ColliderComponent, Components::TransformComponent>([&](const Entity otherEnt, const Components::ColliderComponent *othercCol, const Components::TransformComponent *otherTrans) {
            if (blocked) {
                return;
            }

            const EntityID otherId = static_cast<EntityID>(otherEnt);
            if (otherId == entity || othercCol->isTrigger) {
                return;
            }

            if (!Components::IsValidCollider(*othercCol)) {
                blocked = true;
                return;
            }


            const WorldCollider other = BuildWorldCollider(otherId, *othercCol, *otherTrans, basis);

            if (!OverlapsAABB(candBounds, ComputeWorldAABB(other))) {
                return;
            }

            const GJKResult result = IntersectsGJK(cand, other);

            blocked = result != GJKResult::Separated;
        });

        return !blocked;
    }


} // namespace ECS::Collision
