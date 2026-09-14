#pragma once

#include "ECS/Components/SpriteSheetComponent.h"
#include "ECS/Entity.h"
#include "ECS/ECS.h"

namespace ECS::Systems::SpriteAnimation {
    inline void Update(Registry &registry, const float dt) noexcept {
        registry.ForEach<Components::SpriteSheetComponent>([&](const Entity, Components::SpriteSheetComponent *anim) {
            if (!anim->playing || anim->frameCount <= 1)
                return;

            anim->elapsed += dt;
            const float frameTime = anim->framesPerSecond > 0.0f ? 1.0f / anim->framesPerSecond : 0.0f;
            if (frameTime <= 0.0f)
                return;

            while (anim->elapsed >= frameTime) {
                anim->elapsed -= frameTime;
                anim->currentFrame++;

                if (anim->currentFrame >= anim->frameCount) {
                    if (anim->loop) {
                        anim->currentFrame = 0;
                    } else {
                        anim->currentFrame = anim->frameCount - 1;
                        anim->playing = false;
                        anim->elapsed = 0.0f;
                        break;
                    }
                }
            }
        });
    }
} // namespace ECS::Systems::SpriteAnimation
