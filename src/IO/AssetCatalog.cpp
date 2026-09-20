#include "AssetCatalog.h"
#include <nlohmann/json.hpp>
#include "AssetCatalogFile.h"
#include "Core/ResourceManager.h"
#include "ECS/Systems/Animation/Types.h"
#include "IO/Loaders/AssetLoader.h"
#include "IO/VFS/VFS.h"
#include "Logger/LoggerService.h"
#include "Rendering/Types/Material.h"
#include "Rendering/Types/Mesh/Mesh.h"
#include "Rendering/Types/Shader/Shader.h"
#include "Rendering/Types/Texture/Texture.h"
#include "UI/Text/Font.h"
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace IO::AssetCatalog {
    using json = nlohmann::json;
    namespace {
        bool IsUserAsset(const std::string &id) { return !id.starts_with("[Engine]") && !id.starts_with("[Engine_PP]"); }
        struct IncludeAll {
            bool operator()(const std::string &) const { return true; }
        };
        template <class T, class Func, class Pred = IncludeAll> void SerializeAssets(json &out, std::vector<std::pair<std::string, std::shared_ptr<T>>> assets, Func serializer, Pred predicate = {}) {
            std::sort(assets.begin(), assets.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
            for (const auto &[id, asset] : assets) {
                if (!predicate(id))
                    continue;
                if (!asset)
                    throw std::runtime_error("Null asset: " + id);
                out.push_back(serializer(id, asset));
            }
        }
    } // namespace

    json Serialize() {
        json j;
        auto &resources = Core::ResourceManager::GetInstance();
        // ASSETS
        j["assets"]["textures"] = json::array();
        j["assets"]["shaders"] = json::array();
        j["assets"]["meshes"] = json::array();
        j["assets"]["materials"] = json::array();
        j["assets"]["fonts"] = json::array();
        j["assets"]["animation_sets"] = json::array();

        SerializeAssets(j["assets"]["textures"], resources.GetAll<Rendering::Texture>(), [](const std::string &id, const std::shared_ptr<Rendering::Texture> &tex) { return json{{"id", id}, {"path", tex->GetPath()}}; });

        SerializeAssets(
                j["assets"]["shaders"], resources.GetAll<Rendering::Shader>(),
                [](const std::string &id, const std::shared_ptr<Rendering::Shader> &shad) { return json{{"id", id}, {"vertex", shad->GetVertexPath()}, {"fragment", shad->GetFragmentPath()}}; }, IsUserAsset);

        SerializeAssets(
                j["assets"]["meshes"], resources.GetAll<Rendering::Mesh>(),
                [](const std::string &id, const std::shared_ptr<Rendering::Mesh> &mesh) {
                    json entry = {{"id", id}, {"factory", mesh->GetFactoryId()}};
                    if (mesh->IsCustom()) {
                        const auto data = mesh->GetCustomData();
                        entry["vertices"] = json::array();
                        for (const auto &v : data.vertices) {
                            entry["vertices"].push_back({{"position", {v.Position.x, v.Position.y, v.Position.z}}, {"uv", {v.UV.x, v.UV.y}}});
                        }
                        entry["indices"] = data.indices;
                    }
                    return entry;
                },
                IsUserAsset);

        SerializeAssets(j["assets"]["materials"], resources.GetAll<Rendering::Material>(), [&](const std::string &id, const std::shared_ptr<Rendering::Material> &mat) {
            return json{{"id", id},
                        {"shader", mat->shader ? resources.GetKey(mat->shader) : "[Engine] Base"},
                        {"texture", resources.GetKey(mat->texture)},
                        {"color", {mat->color.r, mat->color.g, mat->color.b, mat->color.a}}};
        });


        SerializeAssets(j["assets"]["fonts"], resources.GetAll<UI::Font>(), [&](const std::string &id, const std::shared_ptr<UI::Font> &font) {
            return json{{"id", id}, {"path", font->GetPath()}, {"size", font->GetFontSize()}, {"sdf", font->IsSDF()}, {"spread", font->GetSDFSpread()}};
        });

        SerializeAssets(j["assets"]["animation_sets"], resources.GetAll<Animation::SpriteAnimationSet>(),
                        [](const std::string &id, const std::shared_ptr<Animation::SpriteAnimationSet> &animation) { return json{{"id", id}, {"path", animation->path.generic_string()}}; });


        return j["assets"];
    }

    bool Load() {
        try {
            auto catalog = VFS::ReadVirtualJson("assets.json");
            if (!catalog)
                throw std::runtime_error("Could not read assets.json");
            auto normalized = CatalogFile::Normalize(*catalog, "assets.json");
            AssetLoader::LoadAssets(normalized.at("assets"), Core::ResourceManager::GetInstance());
            return true;
        } catch (const std::exception &e) {
            LOG_ERROR("AssetCatalog", std::string("Load failed: ") + e.what());
            return false;
        }
    }

    bool Save() {
        if (VFS::IsPackaged() || !VFS::IsProjectLoaded())
            return false;
        try {
            const auto catalog = CatalogFile::Normalize(json{{"version", 1}, {"assets", Serialize()}}, "resource cache");
            CatalogFile::WriteAtomic(VFS::Resolve("assets.json"), catalog);
            return true;
        } catch (const std::exception &e) {
            LOG_ERROR("AssetCatalog", std::string("Save failed: ") + e.what());
            return false;
        }
    }
} // namespace IO::AssetCatalog
