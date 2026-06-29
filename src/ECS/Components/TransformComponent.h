#pragma once

#include "Rendering/Transform.h"

namespace ECS::Components {
    struct TransformComponent {
        Rendering::Transform transform{};
    };
} // namespace ECS::Components

