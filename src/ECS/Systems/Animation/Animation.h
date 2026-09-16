#include "ECS/Components/SpriteAnimatorComponent.h"
#include <cmath>
#include <vector>

namespace Animation {
    inline void Advance(ECS::Components::SpriteAnimatorComponent &player, const SpriteClip &clip, double dt) {
        // clip must  be proper

        if (!player.playing || !std::isfinite(dt) || dt <= 0.0) {
            return;
        }

        if (clip.loop) {
            double cycleDur = 0.0;
            for (const auto &frame : clip.frames) {
                cycleDur += frame.duration;
                dt = std::fmod(dt, cycleDur);
            }

            player.elapsed += dt;

            while (player.elapsed >= clip.frames[player.frameIndex].duration) {
                player.elapsed -= clip.frames[player.frameIndex].duration;

                if (player.frameIndex + 1 < clip.frames.size()) {
                    ++player.frameIndex;
                } else if (clip.loop) {
                    player.frameIndex = 0;
                } else {
                    // hold on last
                    player.playing = false;
                    player.elapsed = 0.0;
                    break;
                }
            }
        }
    }

} // namespace Animation
