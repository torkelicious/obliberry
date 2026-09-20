#pragma once

#include <nlohmann/json_fwd.hpp>

namespace IO::SceneAssetLoader {
    bool LoadReferenced(const nlohmann::json &sceneData);
}
