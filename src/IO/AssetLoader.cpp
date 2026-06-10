#include "AssetLoader.h"

#include <iostream>
#include <stdexcept>
#include "Renderer/Mesh.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture.h"

std::unordered_map<std::string, AssetLoader::MeshFactory>
AssetLoader::s_MeshFactories;

void AssetLoader::LoadAssets(
    const json &assets,
    ResourceManager &resources) {
    if (assets.contains("textures"))
        LoadTextures(assets["textures"], resources);

    if (assets.contains("shaders"))
        LoadShaders(assets["shaders"], resources);

    if (assets.contains("meshes"))
        LoadMeshes(assets["meshes"], resources);

    if (assets.contains("materials"))
        LoadMaterials(assets["materials"], resources);
}

void AssetLoader::RegisterMeshFactory(
    const std::string &name,
    MeshFactory factory) {
    s_MeshFactories[name] = std::move(factory);
}

void AssetLoader::LoadTextures(
    const json &textures,
    ResourceManager &resources) {
    for (const auto &tex: textures) {
        resources.Load<Texture>(
            tex.at("id").get<std::string>(),
            tex.at("path").get<std::string>()
        );
    }
}

void AssetLoader::LoadShaders(
    const json &shaders,
    ResourceManager &resources) {
    for (const auto &shader: shaders) {
        resources.Load<Shader>(
            shader.at("id").get<std::string>(),
            shader.at("vertex").get<std::string>(),
            shader.at("fragment").get<std::string>()
        );
    }
}

void AssetLoader::LoadMaterials(
    const json &materials,
    ResourceManager &resources) {
    for (const auto &mat: materials) {
        const std::string id = mat.at("id").get<std::string>();
        const std::string shaderId = mat.at("shader").get<std::string>();
        const std::string textureId = mat.at("texture").get<std::string>();

        auto shader = resources.Get<Shader>(shaderId);
        auto texture = resources.Get<Texture>(textureId);

        if (!shader)
            throw std::runtime_error("Missing shader: " + shaderId);

        if (!texture)
            throw std::runtime_error("Missing texture: " + textureId);

        glm::vec4 color(1.f);

        if (mat.contains("color")) {
            auto &c = mat["color"];
            color = {
                c[0], c[1], c[2], c[3]
            };
        }

        resources.LoadFromFactory<Material>(
            id,
            [shader, texture, color]() {
                return std::make_shared<Material>(
                    Material{shader, texture, color}
                );
            }
        );
    }
}

void AssetLoader::LoadMeshes(
    const json &meshes,
    ResourceManager &resources) {
    for (const auto &mesh: meshes) {
        const std::string id = mesh.at("id").get<std::string>();
        const std::string factoryName = mesh.at("factory").get<std::string>();

        auto it = s_MeshFactories.find(factoryName);
        if (it == s_MeshFactories.end()) {
            throw std::runtime_error(
                "AssetLoader: Unknown mesh factory '" + factoryName + "'");
        }
        resources.LoadFromFactory<Mesh>(
            id,
            [factory = it->second, factoryName]() {
                auto mesh = factory();
                mesh->SetFactoryId(factoryName);
                return mesh;
            }
        );
    }
}
