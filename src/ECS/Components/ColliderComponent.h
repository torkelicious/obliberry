#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace ECS::Components {

    enum class ColliderShape : uint8_t { Box, Circle };

    struct ColliderComponent {
        ColliderShape shape = ColliderShape::Box;
        glm::vec2 offset{0.0f, 0.0f};
        glm::vec2 size{1.0f};

        float radius = 0.5f; // only applies to circles

        bool isTrigger = false;
    };
} // namespace ECS::Components
