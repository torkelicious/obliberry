#pragma once

#include <string>

#include "json.hpp"

class Scene;

namespace SceneIO {
    bool Deserialize(const std::string &path, Scene &scene);

    bool Serialize(const std::string &path, Scene &scene);

    template<typename T, typename Func>
    void SerializeAssets(
        nlohmann::json &arr,
        const std::unordered_map<std::string, std::shared_ptr<T> > &assets,
        Func serializer) {
        for (const auto &[id, asset]: assets) {
            arr.push_back(serializer(id, asset));
        }
    }
}

