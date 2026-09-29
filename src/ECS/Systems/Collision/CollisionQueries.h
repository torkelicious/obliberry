#pragma once

#include "ColliderGeometry.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Entity.h"
#include "ECS/Systems/Collision/CollisionFilter.h"
#include "ECS/Systems/HierarchySystem.h"
#include "ECS/Types.h"
#include "GJK.h"
#include "ECS/Registry.h"
#include "Math/GLMUtils.h"
#include "glm/ext/vector_float3.hpp"

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
            return true; // nonblocking failure ig
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
                return;
            }

            if (!shouldCollide(*collider, *othercCol)) {
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

    inline bool TryMoveTo(Registry &registry, const EntityID id, const glm::vec3 &targetWorldPos, const BillboardBasis &basis) {
        if (!registry.IsValid(id)) {
            return false;
        }

        if (!Math::GLMUtils::VecIsFinite(targetWorldPos)) {
            return false;
        }

        auto *transform = registry.GetComponent<Components::TransformComponent>(id);
        if (!transform) {
            return false;
        }

        const auto *relationship = registry.GetComponent<Components::RelationshipComponent>(id);
        if (relationship && relationship->parent != INVALID_ENTITY_ID) {
            return false;
        }

        Systems::HierarchySystem::Propagate(registry);

        if (!CanOccupy(registry, id, targetWorldPos, basis)) {
            return false;
        }

        transform->transform.SetPosition(targetWorldPos);

        Systems::HierarchySystem::Propagate(registry);

        return true;
    }


} // namespace ECS::Collision
