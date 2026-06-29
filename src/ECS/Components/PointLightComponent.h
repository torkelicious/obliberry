#pragma once

#include <glm/glm.hpp>

namespace ECS::Components {
    struct PointLightComponent {
        glm::vec3 color = {1.0f, 1.0f, 1.0f};
        float radius = 50.0f; // world space radius
        float intensity = 1.0f;
    };
} // namespace ECS::Components
