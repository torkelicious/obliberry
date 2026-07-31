#pragma once

#include <nlohmann/json.hpp>
#include "Core/ResourceManager.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "Rendering/Material.h"
#include "Rendering/Mesh.h"

namespace IO {
    class AssetLoader {
        using MaterialFactory = std::function<std::shared_ptr<Rendering::Material>()>;

        static void LoadMaterials(const nlohmann::json &materials, Core::ResourceManager &resources);

    public:
        using json = nlohmann::json;
        using MeshFactory = std::function<std::shared_ptr<Rendering::Mesh>()>;

        static void LoadAssets(const json &assets, Core::ResourceManager &resources);

        static void RegisterMeshFactory(const std::string &name, MeshFactory factory);

        static std::optional<std::string> ImportAsset(const std::string &AbsoultePath, const std::string &TargetSubDir = "");

    private:
        static void LoadTextures(const json &textures, Core::ResourceManager &resources);

        static void LoadShaders(const json &shaders, Core::ResourceManager &resources);

        static void LoadMeshes(const json &meshes, Core::ResourceManager &resources);
        static void LoadFonts(const json &fonts, Core::ResourceManager &resources);

        static std::unordered_map<std::string, MeshFactory> s_MeshFactories;
    };
} // namespace IO
