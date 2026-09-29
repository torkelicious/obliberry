#pragma once

#include "ECS/Components/ColliderComponent.h"
#include <cstdint>

namespace ECS::Collision {
    inline bool shouldCollide(const Components::ColliderComponent &a, const Components::ColliderComponent &b) {
        if (a.layer >= 32 || b.layer >= 32) {
            return false;
        }

        const uint32_t aBit = 1u << a.layer;
        const uint32_t bBit = 1u << b.layer;

        const bool aAcceptB = (a.mask & bBit) != 0;
        const bool bAcceptA = (b.mask & aBit) != 0;

        return aAcceptB && bAcceptA;
    }

} // namespace ECS::Collision
