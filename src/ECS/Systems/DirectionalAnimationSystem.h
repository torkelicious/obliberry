#pragma once


#include <cmath>
#include <glm/glm.hpp>
#include "ECS/Entity.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"

// used by both PlayerControlSystem and AISystem.
namespace DirectionalAnimation {
    inline void UpdateFacing(const Entity entity, const glm::vec2 direction, const MaterialComponent *) noexcept {
        if (glm::dot(direction, direction) <= 1.0e-6f) return;

        float degrees = glm::degrees(std::atan2(direction.y, direction.x));
        if (degrees < 0.0f) degrees += 360.0f;
        const int index = static_cast<int>(std::lround(degrees / 60.0f)) % 6;

        auto *dir = entity.GetComponent<DirectionalTextureComponent>();
        if (!dir) return;

        dir->index = index;
    }
}

