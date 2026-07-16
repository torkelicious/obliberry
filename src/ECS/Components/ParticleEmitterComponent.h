#pragma once

#include "Rendering/ParticlePool.h"
#include <memory>

namespace ECS::Components {

    struct ParticleEmitterComponent {
        int emitterIndex = -1;

        int maxParticles = 256;
        float emitRate = 50.0f;
        float lifetimeMin = 0.5f;
        float lifetimeMax = 1.5f;
        glm::vec3 velocityMin = {-1.0f, 2.0f, 0.0f};
        glm::vec3 velocityMax = {1.0f, 5.0f, 0.0f};
        glm::vec3 gravity = {0.0f, -9.8f, 0.0f};
        float sizeStart = 0.2f;
        float sizeEnd = 0.0f;
        glm::vec4 colorStart = {1.0f, 0.8f, 0.2f, 1.0f};
        glm::vec4 colorEnd = {1.0f, 0.2f, 0.0f, 0.0f};
        bool isBillboard = false;

        std::shared_ptr<Rendering::Material> material = nullptr;

        float emitAccumulator = 0.0f;
        bool active = true;
    };

} // namespace ECS::Components
