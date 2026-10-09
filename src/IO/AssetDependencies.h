#pragma once

#include <nlohmann/json_fwd.hpp>
#include <set>
#include <string>

namespace IO::AssetDependencies {
    struct RequiredAssets {
        std::set<std::string> textures;
        std::set<std::string> shaders;
        std::set<std::string> meshes;
        std::set<std::string> materials;
        std::set<std::string> fonts;
        std::set<std::string> animationSets;
    };

    bool IsCatalogAsset(const std::string &id);

    RequiredAssets CollectDirectRefs(const nlohmann::json &scene);

    bool ResolveDependencies(RequiredAssets &required);

    bool BuildAssetSubset(const RequiredAssets &required, nlohmann::json &subset);
} // namespace IO::AssetDependencies
