#pragma once

#include "Animation.h"
#include "ECS/Components/SpriteSheetComponent.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"

namespace ECS::Systems::SpriteAnimation {
    inline void Update(Registry &registry, const float dt) noexcept {
        registry.ForEach<Components::SpriteAnimatorComponent, Components::SpriteSheetComponent>([&](const Entity, Components::SpriteAnimatorComponent *player, Components::SpriteSheetComponent *sprite) {
            if (!player->animations || !player->animations->sheet) {
                return;
            }

            const auto it = player->animations->clips.find(player->clip);

            if (it == player->animations->clips.end()) {
                return;
            }

            const auto &clip = it->second;
            if (clip.frames.empty()) {
                return;
            }

            if (player->frameIndex >= clip.frames.size()) {
                player->frameIndex = 0;
                player->elapsed = 0.0;
            }

            Animation::Advance(*player, clip, dt);
            Animation::ResolvePose(*player, *sprite);
        });
    }
} // namespace ECS::Systems::SpriteAnimation
