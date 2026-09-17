#pragma once

#include <memory>
#include "Rendering/Types/Texture/SpriteSheet.h"

namespace ECS::Components {
    struct SpriteSheetComponent {
        std::shared_ptr<Rendering::SpriteSheet> sheet;
        uint32_t frame = 0;
    };
} // namespace ECS::Components
