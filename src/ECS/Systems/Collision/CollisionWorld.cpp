#include "CollisionWorld.h"
#include "ECS/Registry.h"

#include <utility>

namespace ECS::Systems {
    CollisionWorld::Pair CollisionWorld::MakePair(const EntityID a, const EntityID b) { return a < b ? Pair{a, b} : Pair{b, a}; }

    void CollisionWorld::Update(Registry &registry) {
        CollisionSystem::Update(registry, m_Collisions);

        GenerateEvents();
    }

    void CollisionWorld::GenerateEvents() {
        m_Events.clear();

        CollisionMap currentCollisions;
        currentCollisions.reserve(m_Collisions.size());

        for (const auto &collision : m_Collisions) {
            const Pair pair = MakePair(collision.entityA, collision.entityB);

            const auto previous = m_PreviousCollisions.find(pair);
            const bool existed = previous != m_PreviousCollisions.end();
            const bool changedType = existed && previous->second.isTrigger != collision.isTrigger;

            if (changedType) {
                const auto &old = previous->second;
                m_Events.push_back({.type = CollisionEventType::Exit, .entityA = old.entityA, .entityB = old.entityB, .normal = old.normal, .penetration = 0.0f, .isTrigger = old.isTrigger});
            }
            const CollisionEventType type = existed && !changedType ? CollisionEventType::Stay : CollisionEventType::Enter;

            m_Events.push_back({.type = type, .entityA = collision.entityA, .entityB = collision.entityB, .normal = collision.normal, .penetration = collision.penetration, .isTrigger = collision.isTrigger});

            currentCollisions.emplace(pair, collision);
        }

        for (const auto &[pair, previous] : m_PreviousCollisions) {
            if (currentCollisions.contains(pair))
                continue;

            m_Events.push_back({.type = CollisionEventType::Exit, .entityA = previous.entityA, .entityB = previous.entityB, .normal = previous.normal, .penetration = 0.0f, .isTrigger = previous.isTrigger});
        }

        m_PreviousCollisions = std::move(currentCollisions);
    }

    void CollisionWorld::Clear() {
        m_Collisions.clear();
        m_Events.clear();
        m_PreviousCollisions.clear();
    }
} // namespace ECS::Systems
