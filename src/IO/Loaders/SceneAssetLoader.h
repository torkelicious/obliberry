#pragma once

#include <nlohmann/json_fwd.hpp>

namespace Core {
    class ResourceManager;
}

namespace IO::SceneAssetLoader {
    bool LoadReferenced(const nlohmann::json &sceneData);
}
