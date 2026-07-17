#pragma once

#include <string>

#include <nlohmann/json.hpp>
#include "Core/Utils/JsonUtils.h"

namespace Scenes {
    class Scene;
}

namespace IO {
    // Forward
    inline void RoundJsonFloats(nlohmann::json &j, const int decimals = 3) {
        Core::Utils::Json::RoundJsonFloats(j, decimals);
    }
}

namespace IO::SceneIO {

    bool Deserialize(const std::string &path, Scenes::Scene &scene);

    bool Serialize(const std::string &path, Scenes::Scene &scene);

    inline bool IsUserAsset(const std::string &id) { return !id.starts_with("[Engine]"); }

    template <typename T, typename Func, typename Pred = decltype([](const std::string &) { return true; })>
    void SerializeAssets(nlohmann::json &arr, const std::unordered_map<std::string, std::shared_ptr<T>> &assets, Func serializer, Pred pred = {}) {
        for (const auto &[id, asset] : assets) {
            if (!pred(id))
                continue;
            arr.push_back(serializer(id, asset));
        }
    }
} // namespace IO::SceneIO
