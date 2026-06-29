#pragma once

#include "Rendering/Material.h"

namespace ECS::Components {
    struct MaterialComponent {
        std::shared_ptr<Rendering::Material> material;
    };
} // namespace ECS::Components
