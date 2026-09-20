#pragma once

#include <string>

#include <nlohmann/json.hpp>
#include "Core/Utils/JsonUtils.h"

namespace Scenes {
    class Scene;
}

namespace IO {
    // Forward
    inline void RoundJsonFloats(nlohmann::json &j, const int decimals = 3) { Core::Utils::Json::RoundJsonFloats(j, decimals); }
} // namespace IO

namespace IO::SceneIO {

    bool Deserialize(const std::string &path, Scenes::Scene &scene);

    bool Serialize(const std::string &path, Scenes::Scene &scene);

} // namespace IO::SceneIO
