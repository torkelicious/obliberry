#pragma once

#include <glm/glm.hpp>

namespace ECS::Components {
    struct PointLightComponent {
        glm::vec3 color = {1.0f, 1.0f, 1.0f};
        float radius = 50.0f; // world space radius
        float intensity = 1.0f;
        bool dirty = true;

        void SetIntensity(const float v) {
            intensity = v;
            dirty = true;
        }

        void SetRadius(const float v) {
            radius = v;
            dirty = true;
        }

        void SetColor(const glm::vec3 &c) {
            color = c;
            dirty = true;
        }
    };
} // namespace ECS::Components
