#include "AssetLoader.h"

#include <iostream>
#include <stdexcept>

#include "VFS.h"
#include "Rendering/Mesh.h"
#include "Rendering/Renderer.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"

std::unordered_map<std::string, IO::AssetLoader::MeshFactory>
IO::AssetLoader::s_MeshFactories;

std::optional<std::string> IO::AssetLoader::ImportAsset(const std::string &AbsoultePath,
                                                        const std::string &TargetSubDir) {
    const std::filesystem::path srcpath(AbsoultePath);

    const std::filesystem::path projectAssetsDir = VFS::GetAssetsDirectory();
    if (projectAssetsDir.empty()) {
        std::cerr << "[AssetLoader] Import failed. No project is currently mounted\n";
        return std::nullopt;
    }

    const std::filesystem::path assetdir = VFS::GetAssetsDirectory();
    if (assetdir.empty()) {
        std::cerr << "[AssetLoader] Import failed. "
                "Could not resolve Asset Path.\n Is project loaded?\n";
        return std::nullopt;
    }

    // build fin target
    std::filesystem::path destinationdir = projectAssetsDir;
    if (!TargetSubDir.empty()) {
        destinationdir /= TargetSubDir;
    }

    // ensure exists
    std::filesystem::create_directories(destinationdir);

    // absolute path
    const std::filesystem::path destinationPath = destinationdir / srcpath.filename();

    // copy the file
    try {
        std::filesystem::copy(srcpath, destinationPath, std::filesystem::copy_options::overwrite_existing);
        std::cout << "[AssetLoader] Successfully imported asset to: " << destinationPath.string() << "\n";
    } catch (const std::exception &e) {
        std::cerr << "[AssetLoader] Failed to copy asset: " << e.what() << "\n";
        return std::nullopt;
    }

    // return vfs-resolved path for later serialization
    const std::filesystem::path vfsRelativePath = std::filesystem::relative(destinationPath, VFS::GetProjectRoot());

    // fuck windows and their backslashes :)
    std::string finalVirtualPath = vfsRelativePath.string();
    std::ranges::replace(finalVirtualPath, '\\', '/');

    return finalVirtualPath;
}

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
