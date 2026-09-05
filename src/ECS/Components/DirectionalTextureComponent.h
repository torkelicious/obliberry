#pragma once

#include <array>
#include <memory>

#include "Rendering/Types/Texture/Texture.h"

namespace ECS::Components {
    // for 6-way rotation sprite sheet stuff
    struct DirectionalTextureComponent {
        std::array<std::shared_ptr<Rendering::Texture>, 6> textures;
        uint8_t index = 0;
    };
} // namespace ECS::Components
