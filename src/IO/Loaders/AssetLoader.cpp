#include "AssetLoader.h"
#include "Platform/Threading/SmallTask.h"

#include <stdexcept>
#include <thread>
#include "Logger/LoggerService.h"
#include "IO/VFS/VFS.h"
#include "Rendering/Mesh.h"
#include "Rendering/Renderer.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include "UI/Text/Font.h"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "AssetLoader"

std::unordered_map<std::string, IO::AssetLoader::MeshFactory> IO::AssetLoader::s_MeshFactories;

std::optional<std::string> IO::AssetLoader::ImportAsset(const std::string &AbsoultePath, const std::string &TargetSubDir) {
    const std::filesystem::path srcpath(AbsoultePath);

    const std::filesystem::path projectAssetsDir = VFS::GetAssetsDirectory();
    if (projectAssetsDir.empty()) {
        LOG_ERROR(LOG_WHO, "Import failed. No project is currently mounted");
        return std::nullopt;
    }

    if (const std::filesystem::path assetdir = VFS::GetAssetsDirectory(); assetdir.empty()) {
        LOG_ERROR(LOG_WHO, "Import failed. Could not resolve Asset Path. Is project loaded?");
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

    if (std::filesystem::exists(destinationPath) && std::filesystem::exists(srcpath) && std::filesystem::equivalent(srcpath, destinationPath)) {
        LOG_INFO(LOG_WHO, "Asset already exists at destination: " + destinationPath.string());
    } else {
        try {
            std::filesystem::copy(srcpath, destinationPath, std::filesystem::copy_options::overwrite_existing);
            LOG_INFO(LOG_WHO, "Successfully imported asset to: " + destinationPath.string());
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to copy asset: " + std::string(e.what()));
            return std::nullopt;
        }
    }

    // return vfs-resolved path for later serialization
    const std::filesystem::path vfsRelativePath = std::filesystem::relative(destinationPath, VFS::GetProjectRoot());

    // fuck windows and their backslashes :)
    std::string finalVirtualPath = vfsRelativePath.string();
    std::ranges::replace(finalVirtualPath, '\\', '/');

    return finalVirtualPath;
}

void IO::AssetLoader::LoadAssets(const json &assets, Core::ResourceManager &resources) {
    if (assets.contains("textures"))
        LoadTextures(assets["textures"], resources);

    if (assets.contains("shaders"))
        LoadShaders(assets["shaders"], resources);

    if (assets.contains("meshes"))
        LoadMeshes(assets["meshes"], resources);

    if (assets.contains("materials"))
        LoadMaterials(assets["materials"], resources);

    if (assets.contains("fonts"))
        LoadFonts(assets["fonts"], resources);
}

void IO::AssetLoader::RegisterMeshFactory(const std::string &name, MeshFactory factory) { s_MeshFactories[name] = std::move(factory); }

void IO::AssetLoader::LoadTextures(const json &textures, Core::ResourceManager &resources) {
    for (const auto &tex : textures) {
        const std::string id = tex.at("id").get<std::string>();

        if (resources.Get<Rendering::Texture>(id) != nullptr) {
            continue;
        }

        auto texture = resources.Load<Rendering::Texture>(id, IO::VFS::ToRelative(tex.at("path").get<std::string>()));
        Rendering::Renderer::SubmitInitTask(Platform::Threading::SmallTask([texture] { texture->InitGL(); }));
    }
}


void IO::AssetLoader::LoadShaders(const json &shaders, Core::ResourceManager &resources) {
    for (const auto &shader : shaders) {
        const std::string id = shader.at("id").get<std::string>();

        if (resources.Get<Rendering::Shader>(id) != nullptr) {
            continue;
        }

        auto s = resources.Load<Rendering::Shader>(id, IO::VFS::ToRelative(shader.at("vertex").get<std::string>()), IO::VFS::ToRelative(shader.at("fragment").get<std::string>()));
        Rendering::Renderer::SubmitInitTask(Platform::Threading::SmallTask([s] { s->InitGL(); }));
    }
}


void IO::AssetLoader::LoadFonts(const json &fonts, Core::ResourceManager &resources) {
    for (const auto &font : fonts) {
        const std::string id = font.at("id").get<std::string>();

        if (resources.Get<UI::Font>(id) != nullptr) {
            continue;
        }

        const std::string path = IO::VFS::ToRelative(font.at("path").get<std::string>());
        const unsigned int size = font.value("size", 12);
        const bool useSDF = font.value("sdf", false);
        const unsigned int spread = font.value("spread", 8);

        auto f = resources.Load<UI::Font>(id, path, size, useSDF, spread);
        // defer font atlas generation to a background thread to avoid blocking the main thread
        std::thread([f] {
            f->LoadCPU();
            Rendering::Renderer::SubmitInitTask(Platform::Threading::SmallTask([f] { f->InitGL(); }));
        }).detach();
    }
}

void IO::AssetLoader::LoadMaterials(const json &materials, Core::ResourceManager &resources) {
    for (const auto &mat : materials) {
        const std::string id = mat.at("id").get<std::string>();
        const std::string shaderId = mat.at("shader").get<std::string>();
        const std::string textureId = mat.at("texture").get<std::string>();

        auto shader = resources.Get<Rendering::Shader>(shaderId);
        auto texture = resources.Get<Rendering::Texture>(textureId);

        if (!shader)
            LOG_ERROR(LOG_WHO, "Missing shader: " + shaderId);

        if (!texture)
            texture = nullptr;

        glm::vec4 color(1.f);
        if (mat.contains("color")) {
            auto &c = mat["color"];
            color = {c[0], c[1], c[2], c[3]};
        }

        resources.LoadFromFactory<Rendering::Material>(id, [shader, texture, color] { return std::make_shared<Rendering::Material>(Rendering::Material{shader, texture, color}); });
    }
}

void IO::AssetLoader::LoadMeshes(const json &meshes, Core::ResourceManager &resources) {
    for (const auto &mesh : meshes) {
        const std::string id = mesh.at("id").get<std::string>();

        if (resources.Get<Rendering::Mesh>(id) != nullptr) {
            continue;
        }

        const std::string factoryName = mesh.at("factory").get<std::string>();
        auto it = s_MeshFactories.find(factoryName);
        if (it == s_MeshFactories.end()) {
            throw std::runtime_error("AssetLoader: Unknown mesh factory '" + factoryName + "'");
        }

        auto m = resources.LoadFromFactory<Rendering::Mesh>(id, [factory = it->second, factoryName] {
            auto fac_mesh = factory();
            fac_mesh->SetFactoryId(factoryName);
            return fac_mesh;
        });

        Rendering::Renderer::SubmitInitTask(Platform::Threading::SmallTask([m] { m->InitGL(); }));
    }
}
#pragma pop_macro("LOG_WHO")
