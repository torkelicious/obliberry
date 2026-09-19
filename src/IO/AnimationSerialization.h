#pragma once

#include "ECS/Systems/Animation/Types.h"
#include "nlohmann/json_fwd.hpp"
#include <filesystem>
namespace IO::AnimationIO {

    nlohmann::json AnimationToJson(Animation::SpriteAnimationSet &animation);
    Animation::SpriteAnimationSet JsonToAnimation(const nlohmann::json &data);

    bool Serialize(Animation::SpriteAnimationSet &set, const std::filesystem::path &path);
    Animation::SpriteAnimationSet Deserialize(const std::filesystem::path &path);

} // namespace IO::AnimationIO
