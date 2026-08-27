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
    try {
        std::filesystem::create_directories(destinationdir);
    } catch (const std::exception &e) {
        LOG_ERROR(LOG_WHO, "Failed to create asset directory: " + std::string(e.what()));
        return std::nullopt;
    }

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
        try {
            const std::string id = tex.at("id").get<std::string>();

            if (resources.Get<Rendering::Texture>(id) != nullptr) {
                continue;
            }

            auto texture = resources.Load<Rendering::Texture>(id, VFS::ToRelative(tex.at("path").get<std::string>()));
            Rendering::Renderer::SubmitInitTask(Platform::Threading::SmallTask([texture] { texture->InitGL(); }));
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, std::string("Skipping texture asset: ") + e.what());
        }
    }
}


void IO::AssetLoader::LoadShaders(const json &shaders, Core::ResourceManager &resources) {
    for (const auto &shader : shaders) {
        try {
            const std::string id = shader.at("id").get<std::string>();

            if (resources.Get<Rendering::Shader>(id) != nullptr) {
                continue;
            }

            auto s = resources.Load<Rendering::Shader>(id, VFS::ToRelative(shader.at("vertex").get<std::string>()), VFS::ToRelative(shader.at("fragment").get<std::string>()));
            Rendering::Renderer::SubmitInitTask(Platform::Threading::SmallTask([s] { s->InitGL(); }));
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, std::string("Skipping shader asset: ") + e.what());
        }
    }
}


void IO::AssetLoader::LoadFonts(const json &fonts, Core::ResourceManager &resources) {
    for (const auto &font : fonts) {
        try {
            const std::string id = font.at("id").get<std::string>();

            if (resources.Get<UI::Font>(id) != nullptr) {
                continue;
            }

            const std::string path = VFS::ToRelative(font.at("path").get<std::string>());
            const unsigned int size = font.value("size", 12);
            const bool useSDF = font.value("sdf", false);
            const unsigned int spread = font.value("spread", 8);

            auto f = resources.Load<UI::Font>(id, path, size, useSDF, spread);
            // defer font atlas generation to a background thread to avoid blocking the main thread
            std::thread([f] {
                f->LoadCPU();
                Rendering::Renderer::SubmitInitTask(Platform::Threading::SmallTask([f] { f->InitGL(); }));
            }).detach();
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, std::string("Skipping font asset: ") + e.what());
        }
    }
}

void IO::AssetLoader::LoadMaterials(const json &materials, Core::ResourceManager &resources) {
    for (const auto &mat : materials) {
        try {
            const std::string id = mat.at("id").get<std::string>();
            auto shaderId = mat.at("shader").get<std::string>();
            const std::string textureId = mat.at("texture").get<std::string>();

            auto shader = resources.Get<Rendering::Shader>(shaderId);
            if (!shader)
                shader = resources.Get<Rendering::Shader>("[Engine] Base");

            auto texture = resources.Get<Rendering::Texture>(textureId);

            if (!texture)
                texture = nullptr;

            glm::vec4 color(1.f);
            if (mat.contains("color") && mat["color"].is_array() && mat["color"].size() >= 4) {
                auto &c = mat["color"];
                color = {c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>()};
            }

            resources.LoadFromFactory<Rendering::Material>(id, [shader, texture, color] { return std::make_shared<Rendering::Material>(Rendering::Material{.shader = shader, .texture = texture, .color = color}); });
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, std::string("Skipping material asset: ") + e.what());
        }
    }
}

void IO::AssetLoader::LoadMeshes(const json &meshes, Core::ResourceManager &resources) {
    for (const auto &mesh : meshes) {
        try {
            const std::string id = mesh.at("id").get<std::string>();

            if (resources.Get<Rendering::Mesh>(id) != nullptr) {
                continue;
            }

            const std::string factoryName = mesh.at("factory").get<std::string>();
            std::shared_ptr<Rendering::Mesh> meshObj;

            if (factoryName == "Custom") {
                Rendering::MeshData data;
                if (mesh.contains("vertices") && mesh.contains("indices")) {
                    for (const auto &v : mesh.at("vertices")) {
                        data.vertices.push_back(
                                {.Position = {v.at("position")[0].get<float>(), v.at("position")[1].get<float>(), v.at("position")[2].get<float>()}, .UV = {v.at("uv")[0].get<float>(), v.at("uv")[1].get<float>()}});
                    }
                    for (const auto &idx : mesh.at("indices")) {
                        data.indices.push_back(idx.get<uint32_t>());
                    }
                }
                meshObj = std::make_shared<Rendering::Mesh>(data);
                meshObj->SetFactoryId("Custom");
            } else {
                auto it = s_MeshFactories.find(factoryName);
                if (it == s_MeshFactories.end()) {
                    throw std::runtime_error("AssetLoader: Unknown mesh factory '" + factoryName + "'");
                }
                meshObj = resources.LoadFromFactory<Rendering::Mesh>(id, [factory = it->second, factoryName] {
                    auto fac_mesh = factory();
                    fac_mesh->SetFactoryId(factoryName);
                    return fac_mesh;
                });
            }

            if (meshObj) {
                resources.Register<Rendering::Mesh>(id, meshObj);
                Rendering::Renderer::SubmitInitTask(Platform::Threading::SmallTask([meshObj] { meshObj->InitGL(); }));
            }
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, std::string("Skipping mesh asset: ") + e.what());
        }
    }
}
#pragma pop_macro("LOG_WHO")
