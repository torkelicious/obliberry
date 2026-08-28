#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include "Core/Utils/JsonUtils.h"
#include "Rendering/Types/Mesh/Mesh.h"

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

    inline bool IsUserAsset(const std::string &id) { return !id.starts_with("[Engine]"); }

    // vector<pair<...>> returned by ResourceManager::GetAll
    template <typename T, typename Func, typename Pred = decltype([](const std::string &) { return true; })>
    void SerializeAssets(nlohmann::json &arr, const std::vector<std::pair<std::string, std::shared_ptr<T>>> &assets, Func serializer, Pred pred = {}) {
        for (const auto &[id, asset] : assets) {
            if (!pred(id))
                continue;
            arr.push_back(serializer(id, asset));
        }
    }
} // namespace IO::SceneIO
