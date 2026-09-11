
#pragma once

#include "ECS/Systems/Collision/CollisionSystem.h"
#include "ECS/Types.h"

#include <cstdint>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

namespace ECS {
    class Registry;
}

namespace ECS::Systems {
    enum class CollisionEventType : uint8_t { Enter, Stay, Exit };

    struct CollisionEvent {
        CollisionEventType type;

        EntityID entityA = INVALID_ENTITY_ID;
        EntityID entityB = INVALID_ENTITY_ID;

        glm::vec2 normal{0.0f};
        float penetration = 0.0f;

        bool isTrigger = false;
    };

    class CollisionWorld {
    public:
        void Update(Registry &registry);

        [[nodiscard]]
        const std::vector<CollisionSystem::Collision> &GetCollisions() const {
            return m_Collisions;
        }

        [[nodiscard]]
        const std::vector<CollisionEvent> &GetEvents() const {
            return m_Events;
        }

        void Clear();

    private:
        struct Pair {
            EntityID first;
            EntityID second;

            bool operator==(const Pair &) const = default;
        };

        struct PairHash {
            size_t operator()(const Pair &pair) const {
                const uint64_t value = static_cast<uint64_t>(pair.first) << 32 | static_cast<uint64_t>(pair.second);

                return std::hash<uint64_t>{}(value);
            }
        };

        using CollisionMap = std::unordered_map<Pair, CollisionSystem::Collision, PairHash>;

        static Pair MakePair(EntityID a, EntityID b);

        void GenerateEvents();

        std::vector<CollisionSystem::Collision> m_Collisions;
        std::vector<CollisionEvent> m_Events;

        CollisionMap m_PreviousCollisions;
    };
} // namespace ECS::Systems
