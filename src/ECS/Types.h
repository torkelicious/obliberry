#pragma once

#include <cstdint>

namespace ECS {
    using EntityID = uint32_t;
    constexpr uint32_t MAX_ENTITIES = 5000;

    // Generational ID Masks
    constexpr uint32_t ENTITY_INDEX_MASK = 0xFFFFF;      // lower 20 bits for Index
    constexpr uint32_t ENTITY_VERSION_MASK = 0xFFF00000; // upper 12 bits for Version/Gen
    constexpr uint32_t ENTITY_VERSION_SHIFT = 20;
    inline uint32_t GetEntityIndex(const EntityID id) { return id & ENTITY_INDEX_MASK; }
    inline uint32_t GetEntityVersion(const EntityID id) { return (id & ENTITY_VERSION_MASK) >> ENTITY_VERSION_SHIFT; }
} // namespace ECS
