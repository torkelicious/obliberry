#pragma once

#include "Rendering/Types/Material.h"

namespace ECS::Components {
    struct MaterialComponent {
        std::shared_ptr<Rendering::Material> material;
    };
} // namespace ECS::Components
