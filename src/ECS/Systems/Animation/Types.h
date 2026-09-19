#pragma once

#include "Rendering/Types/Texture/SpriteSheet.h"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

namespace Animation {
    struct SpriteFrame {
        uint32_t index = 0;
        float duration = 0.1f;
    };

    struct SpriteClip {
        std::vector<SpriteFrame> frames;
        bool loop = true;
    };

    struct SpriteAnimationSet {
        std::filesystem::path path;
        std::shared_ptr<Rendering::SpriteSheet> sheet;
        std::unordered_map<std::string, SpriteClip> clips;
    };

} // namespace Animation
