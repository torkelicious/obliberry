#pragma once
#include <memory>
#include "Rendering/Types/Texture/SpriteSheet.h"

namespace ECS::Components {
    struct SpriteSheetComponent {
        std::shared_ptr<Rendering::SpriteSheet> sheet = std::make_shared<Rendering::SpriteSheet>();
        int startFrame = 0;
        int frameCount = 1;
        float framesPerSecond = 8.0f;
        bool loop = false;
        bool playing = false;
        float elapsed = 0.0f;
        int currentFrame = 0;
    };
} // namespace ECS::Components
