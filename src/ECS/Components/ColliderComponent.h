#pragma once

#include <glm/glm.hpp>
#include <cmath>
#include <cstdint>

namespace ECS::Components {

    enum class ColliderShape : uint8_t { Box, Sphere, Cylinder, Rectangle, Circle };

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
        if (c.orientation != ColliderOrientation::Entity && c.orientation != ColliderOrientation::Billboard) {
            return false;
        }

        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(c.offset[i])) {
                return false;
            }
        }

        const auto positive = [](const float value) { return std::isfinite(value) && value > 0.0f; };

        switch (c.shape) {
            case ColliderShape::Box:
                return positive(c.size.x) && positive(c.size.y) && positive(c.size.z);

            case ColliderShape::Sphere:
            case ColliderShape::Circle:
                return positive(c.radius);

            case ColliderShape::Cylinder:
                return positive(c.radius) && positive(c.height);

            case ColliderShape::Rectangle:
                return positive(c.size.x) && positive(c.size.y);
        }
        return false;
    }

} // namespace ECS::Components
