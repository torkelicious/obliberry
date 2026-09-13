#pragma once

#include "ECS/Systems/Collision/ColliderGeometry.h"
#include "ECS/Types.h"
#include "ECS/Registry.h"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ECS::Collision {

    struct Collision {
        ECS::EntityID entityA = INVALID_ENTITY_ID;
        ECS::EntityID entityB = INVALID_ENTITY_ID;
        bool isTrigger = false;
    };

    enum class CollisionEventType : uint8_t { Enter, Stay, Exit };

    struct CollisionEvent {
        CollisionEventType type = CollisionEventType::Enter;
        ECS::EntityID entityA = INVALID_ENTITY_ID;
        ECS::EntityID entityB = INVALID_ENTITY_ID;
        bool isTrigger = false;
    };

    class CollisionWorld {
    public:
        void Update(Registry &registry, const BillboardBasis &basis = {});
        const std::vector<Collision> &GetCollisions() const;
        const std::vector<CollisionEvent> &GetEvents() const;
        void Clear();

    private:
        using CollisionMap = std::unordered_map<uint64_t, Collision>;
        CollisionMap m_Previous;
        std::vector<Collision> m_Collisions;
        std::vector<CollisionEvent> m_Events;

        std::unordered_set<EntityID> m_ReportedInvalid;
        std::unordered_set<uint64_t> m_ReportedIndeterminate;

        static uint64_t MakeKey(EntityID a, EntityID b);
        void Emit(CollisionEventType type, const Collision &collision);
        void ReportInvalidCollider(EntityID entity, bool valid);
        void ReportIndeterminate(EntityID a, EntityID b, bool indeterminate);
    };


} // namespace ECS::Collision
