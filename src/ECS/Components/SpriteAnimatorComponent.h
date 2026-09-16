#pragma once

#include <memory>
#include "ECS/Systems/Animation/Types.h"

namespace ECS::Components {

struct SpriteAnimatorComponent {
    std::shared_ptr<const Animation::SpriteAnimationSet> animations;

    // saved config
    std::string initialClip;
    bool autoplay = true;

    std::string clip;
    size_t frameIndex = 0;
    double elapsed = 0.0;
    bool playing = false;
};;


} // namespace ECS::Components
