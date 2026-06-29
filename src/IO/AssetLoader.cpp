#include "AssetLoader.h"

#include <iostream>
#include <stdexcept>
#include "Rendering/Mesh.h"
#include "Rendering/Renderer.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"

std::unordered_map<std::string, IO::AssetLoader::MeshFactory>
IO::AssetLoader::s_MeshFactories;

void IO::AssetLoader::LoadAssets(
    const json &assets,
    Core::ResourceManager &resources) {
    if (assets.contains("textures"))
        LoadTextures(assets["textures"], resources);

    if (assets.contains("shaders"))
        LoadShaders(assets["shaders"], resources);

    if (assets.contains("meshes"))
        LoadMeshes(assets["meshes"], resources);

    if (assets.contains("materials"))
        LoadMaterials(assets["materials"], resources);
}

void IO::AssetLoader::RegisterMeshFactory(
    const std::string &name,
    MeshFactory factory) {
    s_MeshFactories[name] = std::move(factory);
}

void IO::AssetLoader::LoadTextures(
    const json &textures,
    Core::ResourceManager &resources) {
    for (const auto &tex: textures) {
        auto texture = resources.Load<Rendering::Texture>(
            tex.at("id").get<std::string>(),
            tex.at("path").get<std::string>()
        );
        Rendering::Renderer::SubmitInitTask([texture] {
            texture->InitGL();
        });
    }
}


void IO::AssetLoader::LoadShaders(
    const json &shaders,
    Core::ResourceManager &resources) {
    for (const auto &shader: shaders) {
        auto s = resources.Load<Rendering::Shader>(
            shader.at("id").get<std::string>(),
            shader.at("vertex").get<std::string>(),
            shader.at("fragment").get<std::string>()
        );
        Rendering::Renderer::SubmitInitTask([s] {
            s->InitGL();
        });
    }
}


void IO::AssetLoader::LoadMaterials(
    const json &materials,
    Core::ResourceManager &resources) {
    for (const auto &mat: materials) {
        const std::string id = mat.at("id").get<std::string>();
        const std::string shaderId = mat.at("shader").get<std::string>();
        const std::string textureId = mat.at("texture").get<std::string>();

        auto shader = resources.Get<Rendering::Shader>(shaderId);
        auto texture = resources.Get<Rendering::Texture>(textureId);

        if (!shader)
            std::cerr << "Missing shader: " + shaderId + "\n";

        if (!texture)
            texture = nullptr;

        glm::vec4 color(1.f);
        if (mat.contains("color")) {
            auto &c = mat["color"];
            color = {
                c[0], c[1], c[2], c[3]
            };
        }

        resources.LoadFromFactory<Rendering::Material>(
            id,
            [shader, texture, color] {
                return std::make_shared<Rendering::Material>(
                    Rendering::Material{shader, texture, color}
                );
            }
        );
    }
}

void IO::AssetLoader::LoadMeshes(
    const json &meshes,
    Core::ResourceManager &resources) {
    for (const auto &mesh: meshes) {
        const std::string id = mesh.at("id").get<std::string>();
        const std::string factoryName = mesh.at("factory").get<std::string>();
        auto it = s_MeshFactories.find(factoryName);
        if (it == s_MeshFactories.end()) {
            throw std::runtime_error(
                "AssetLoader: Unknown mesh factory '" + factoryName + "'");
        }

        auto m = resources.LoadFromFactory<Rendering::Mesh>(
            id,
            [factory = it->second, factoryName] {
                auto fac_mesh = factory();
                fac_mesh->SetFactoryId(factoryName);
                return fac_mesh;
            }
        );

        Rendering::Renderer::SubmitInitTask([m] {
            m->InitGL();
        });
    }
}
