#pragma once
#include "ECS/Components/SpriteAnimatorComponent.h"
#include "ECS/Components/SpriteSheetComponent.h"
#include "ECS/Systems/Animation/Types.h"
#include "Rendering/Types/Texture/SpriteSheet.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace Animation {

    using Player = ECS::Components::SpriteAnimatorComponent;
    using Sprite = ECS::Components::SpriteSheetComponent;

    enum class RestartSetting : uint8_t { KeepIfSame, Restart };

    inline bool ValidateSheet(Rendering::SpriteSheet &sheet) {
        if (!sheet.texture || sheet.columns <= 0 || sheet.rows <= 0 || sheet.columnSpacing < 0 || sheet.rowSpacing < 0) {
            return false;
        }

        const int64_t cols = sheet.columns;
        const int64_t rows = sheet.rows;

        if (cols * rows > std::numeric_limits<int>::max()) {
            return false;
        }

        const int64_t w = int64_t(sheet.texture->GetWidth() - int64_t(sheet.columnSpacing) * (cols - 1));
        const int64_t h = int64_t(sheet.texture->GetHeight() - int64_t(sheet.rowSpacing) * (rows - 1));

        return w >= cols && h >= rows && w % cols == 0 && h % cols == 0;
    }

    inline bool ValidateClip(const SpriteAnimationSet &set, const SpriteClip &clip) {
        if (!set.sheet || !ValidateSheet(*set.sheet) || clip.frames.empty()) {
            return false;
        }

        const int64_t total = int64_t(set.sheet->columns) * set.sheet->rows;

        for (const auto &frame : clip.frames) {
            if (int64_t(frame.index) >= total || !std::isfinite(frame.duration) || frame.duration <= 0.0f) {
                return false;
            }
        }

        return true;
    }

    inline const SpriteClip *FindClip(const Player &player) {
        if (!player.animations) {
            return nullptr;
        }

        const auto it = player.animations->clips.find(player.clip);
        return it == player.animations->clips.end() ? nullptr : &it->second;
    }

    inline bool Play(Player &player, const std::string &name, RestartSetting setting = RestartSetting::KeepIfSame) {
        if (!player.animations) {
            return false;
        }

        const auto it = player.animations->clips.find(name);
        if (it == player.animations->clips.end() || !ValidateClip(*player.animations, it->second)) {
            return false;
        }

        if (setting == RestartSetting::KeepIfSame && player.clip == name) {
            return true;
        }

        player.clip = name;
        player.frameIndex = 0;
        player.elapsed = 0.0;
        player.playing = true;
        return true;
    }

    inline void Pause(Player &player) { player.playing = false; }

    inline void Resume(Player &player) {
        const auto *clip = FindClip(player);

        if (clip && player.animations && ValidateClip(*player.animations, *clip) && player.frameIndex < clip->frames.size()) {
            player.playing = true;
        }
    }

    inline void Stop(Player &player) {
        player.frameIndex = 0;
        player.elapsed = 0.0;
        player.playing = false;
    }

    inline bool ResetToInitial(Player &player) {
        player.clip.clear();
        Stop(player);

        if (player.initialClip.empty())
            return false;

        if (!Play(player, player.initialClip, RestartSetting::Restart))
            return false;

        player.playing = player.autoplay;
        return true;
    }

    // defl
    inline bool ResolvePose(const Player &player, Sprite &sprite) {
        const auto *clip = FindClip(player);

        if (!player.animations || !player.animations->sheet || !clip || player.frameIndex >= clip->frames.size()) {
            return false;
        }

        sprite.sheet = player.animations->sheet;
        sprite.frame = clip->frames[player.frameIndex].index;
        return true;
    }

    inline void Advance(Player &player, const SpriteClip &clip, double dt) {
        if (!player.playing || clip.frames.empty() || player.frameIndex >= clip.frames.size() || !std::isfinite(dt) || dt <= 0.0) {
            return;
        }

        if (clip.loop) {
            double cycleDuration = 0.0;

            for (const auto &frame : clip.frames)
                cycleDuration += frame.duration;

            dt = std::fmod(dt, cycleDuration);
        }

        player.elapsed += dt;

        while (player.elapsed >= clip.frames[player.frameIndex].duration) {
            player.elapsed -= clip.frames[player.frameIndex].duration;

            if (player.frameIndex + 1 < clip.frames.size()) {
                ++player.frameIndex;
            } else if (clip.loop) {
                player.frameIndex = 0;
            } else {
                player.playing = false;
                player.elapsed = 0.0;
                break;
            }
        }
    }

} // namespace Animation
