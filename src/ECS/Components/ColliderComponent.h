#pragma once

#include <glm/glm.hpp>
#include <cmath>
#include <cstdint>

namespace ECS::Components {

    enum class ColliderShape : uint8_t { Box, Sphere, Cylinder };

    enum class ColliderOrientation : uint8_t { Entity, Billboard };

    struct ColliderComponent {
        ColliderShape shape = ColliderShape::Box;
        ColliderOrientation orientation = ColliderOrientation::Entity;

        glm::vec3 offset{0.0f};

        glm::vec3 size{1.0f}; // full box dimensions
        float radius = 0.5f;  // sphere or cylinder
        float height = 1.0f;  // full cylinder height local y

        bool isTrigger = false;

        bool operator==(const ColliderComponent &other) const = default;
    };

    inline bool IsValidCollider(const ColliderComponent &c) {
        if (c.shape != ColliderShape::Box && c.shape != ColliderShape::Sphere && c.shape != ColliderShape::Cylinder)
            return false;

        if (c.orientation != ColliderOrientation::Entity && c.orientation != ColliderOrientation::Billboard)
            return false;

        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(c.offset[i]) || !std::isfinite(c.size[i]) || c.size[i] <= 0.0f)
                return false;
        }

        return std::isfinite(c.radius) && c.radius > 0.0f && std::isfinite(c.height) && c.height > 0.0f;
    }

} // namespace ECS::Components
