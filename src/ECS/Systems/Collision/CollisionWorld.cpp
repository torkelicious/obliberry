#include "CollisionWorld.h"

#define LOG_WHO "Collision"

#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/Systems/Collision/ColliderGeometry.h"
#include "ECS/Systems/Collision/GJK.h"
#include "Logger/LoggerService.h"
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ECS::Collision {

    void CollisionWorld::Update(Registry &registry, const BillboardBasis &basis) {
        m_Events.clear();
        std::vector<WorldCollider> worldColliders;

        registry.ForEach<Components::ColliderComponent, Components::TransformComponent>([&](const Entity entity, const Components::ColliderComponent *collider, const Components::TransformComponent *transform) {
            const auto entityID = static_cast<EntityID>(entity);

            const bool valid = Components::IsValidCollider(*collider);
            ReportInvalidCollider(entityID, valid);
            if (!valid) {
                return;
            }


            worldColliders.push_back(BuildWorldCollider(entityID, *collider, *transform, basis));
        });

        std::vector<ColliderAABB> aabbs;
        aabbs.reserve(worldColliders.size());

        for (const WorldCollider &collider : worldColliders) {
            aabbs.push_back(ComputeWorldAABB(collider));
        }

        std::vector<Collision> nextCollisions;
        for (size_t i = 0; i < worldColliders.size(); ++i) {
            for (size_t j = i + 1; j < worldColliders.size(); ++j) {
                if (!OverlapsAABB(aabbs[i], aabbs[j])) {
                    continue;
                }

                const auto &a = worldColliders[i];
                const auto &b = worldColliders[j];

                const GJKResult result = IntersectsGJK(a, b);

                ReportIndeterminate(a.entity, b.entity, result == GJKResult::Indeterminate);
                if (result == GJKResult::Indeterminate) {
                    return;
                }

                if (result == GJKResult::Separated) {
                    continue;
                }

                nextCollisions.push_back({std::min(a.entity, b.entity), std::max(a.entity, b.entity), a.geometry.isTrigger || b.geometry.isTrigger});
            }
        }
        CollisionMap nextPairs;
        nextPairs.reserve(nextCollisions.size());

        for (const Collision &collision : nextCollisions) {
            const uint64_t key = MakeKey(collision.entityA, collision.entityB);
            const auto previous = m_Previous.find(key);

            if (previous == m_Previous.end()) {
                Emit(CollisionEventType::Enter, collision);
            } else if (previous->second.isTrigger != collision.isTrigger) {
                Emit(CollisionEventType::Exit, previous->second);
                Emit(CollisionEventType::Enter, collision);
            } else {
                Emit(CollisionEventType::Stay, collision);
            }
            nextPairs.emplace(key, collision);
        }
        for (const auto &[key, previous] : m_Previous) {
            if (!nextPairs.contains(key)) {
                Emit(CollisionEventType::Exit, previous);
            }
        }
        m_Previous = std::move(nextPairs);
        m_Collisions = std::move(nextCollisions);
    }

    const std::vector<CollisionEvent> &CollisionWorld::GetEvents() const { return m_Events; }

    const std::vector<Collision> &CollisionWorld::GetCollisions() const { return m_Collisions; }

    void CollisionWorld::Clear() {
        m_Previous.clear();
        m_Collisions.clear();
        m_Events.clear();
        m_ReportedInvalid.clear();
        m_ReportedIndeterminate.clear();
    }

    uint64_t CollisionWorld::MakeKey(EntityID a, EntityID b) {
        if (a > b) {
            std::swap(a, b);
        }
        return (uint64_t(a) << 32) | uint64_t(b);
    }

    void CollisionWorld::Emit(CollisionEventType type, const Collision &collision) { m_Events.push_back({type, collision.entityA, collision.entityB, collision.isTrigger}); }

    void CollisionWorld::ReportInvalidCollider(EntityID entity, bool valid) {
        if (!valid && m_ReportedInvalid.insert(entity).second) {
            LOG_ERROR(LOG_WHO, "Collision skipped entity with invalid collider: " + std::to_string(entity));
        } else if (valid) {
            m_ReportedInvalid.erase(entity);
        }
    }

    void CollisionWorld::ReportIndeterminate(EntityID a, EntityID b, bool indeterminate) {
        const uint64_t key = MakeKey(a, b);

        if (!indeterminate) {
            m_ReportedIndeterminate.erase(key);
        } else if (m_ReportedIndeterminate.insert(key).second) {
            LOG_ERROR(LOG_WHO, "GJK was indeterminate for pair " + std::to_string(a) + " / " + std::to_string(b) + ", treating as intersecting");
        }
    }

} // namespace ECS::Collision
