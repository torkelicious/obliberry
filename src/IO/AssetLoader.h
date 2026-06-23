#pragma once

#include "json.hpp"
#include "Core/ResourceManager.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "Renderer/Material.h"

class ResourceManager;
class Mesh;

class AssetLoader {
    using MaterialFactory = std::function<std::shared_ptr<Material>()>;

    static void LoadMaterials(const nlohmann::json &materials, ResourceManager &resources);

public:
    using json = nlohmann::json;
    using MeshFactory = std::function<std::shared_ptr<Mesh>()>;

    static void LoadAssets(
        const json &assets,
        ResourceManager &resources);

    static void RegisterMeshFactory(
        const std::string &name,
        MeshFactory factory);

private:
    static void LoadTextures(
        const json &textures,
        ResourceManager &resources);

    static void LoadShaders(
        const json &shaders,
        ResourceManager &resources);

    static void LoadMeshes(
        const json &meshes,
        ResourceManager &resources);

private:
    static std::unordered_map<std::string, MeshFactory>
    s_MeshFactories;
};
