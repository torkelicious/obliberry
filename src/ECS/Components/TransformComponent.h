#pragma once

#include "Rendering/Transform.h"

namespace ECS::Components {
    struct TransformComponent {
        Rendering::Transform transform{};      // local space
        Rendering::Transform worldTransform{}; // world space
    };
} // namespace ECS::Components
