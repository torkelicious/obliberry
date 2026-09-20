#include "SceneAssetLoader.h"
#include "Core/ResourceManager.h"
#include "nlohmann/json.hpp"
#include <cstddef>
#include <functional>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>

namespace {
    using json = nlohmann::json;

    struct RequiredAssets {
        std::set<std::string> textures;
        std::set<std::string> shaders;
        std::set<std::string> meshes;
        std::set<std::string> materials;
        std::set<std::string> fonts;
        std::set<std::string> animationSets;
    };

    bool IsCatalogAsset(const std::string &id) { return !id.empty() && !id.starts_with("[Engine]") && !id.starts_with("[Engine_PP]"); }

    void AddField(std::set<std::string> &destination, const json &obj, const char *field) {
        if (!obj.is_object()) {
            return;
        }

        const auto found = obj.find(field);
        if (found == obj.end() || !found->is_string()) {
            return;
        }

        const std::string id = found->get<std::string>();
        if (IsCatalogAsset(id)) {
            destination.insert(id);
        }
    }


    void AddStrArray(std::set<std::string> &destination, const json &obj, const char *field) {
        if (!obj.is_object()) {
            return;
        }

        const auto found = obj.find(field);
        if (found == obj.end() || !found->is_array()) {
            return;
        }

        for (const auto &value : *found) {
            if (!value.is_string()) {
                continue;
            }
            const std::string id = value.get<std::string>();
            if (IsCatalogAsset(id)) {
                destination.insert(id);
            }
        }
    }

    void CollectUIElement(const json &el, RequiredAssets &req) {
        if (!el.is_object()) {
            return;
        }
        AddField(req.fonts, el, "font");
        AddField(req.textures, el, "texture");
        AddField(req.textures, el, "bg_texture");

        const auto children = el.find("children");
        if (children == el.end() || !children->is_array()) {
            return;
        }

        for (const auto &child : *children) {
            CollectUIElement(child, req);
        }
    }

    void CollectUIReferences(const json &scene, RequiredAssets &req) {
        const auto ui = scene.find("ui");
        if (ui == scene.end() || !ui->is_object()) {
            return;
        }

        const auto elements = ui->find("elements");
        if (elements == ui->end() || !elements->is_array()) {
            return;
        }

        for (const auto &el : *elements) {
            CollectUIElement(el, req);
        }
    }

    void CollectGridReferences(const json &scene, RequiredAssets &req) {
        const auto grid = scene.find("grid");
        if (grid == scene.end() || !grid->is_object()) {
            return;
        }

        AddField(req.meshes, *grid, "mesh_id");

        const auto types = grid->find("types");
        if (types == grid->end() || !types->is_array()) {
            return;
        }

        for (const auto &type : *types) {
            AddField(req.textures, type, "texture");
        }
    }

    void CollectPostProcReferences(const json &scene, RequiredAssets &req) {
        const auto fx = scene.find("PostProcessing");
        if (fx == scene.end() || !fx->is_array()) {
            return;
        }

        for (const auto &effect : *fx) {
            AddField(req.shaders, effect, "shader");
        }
    }

    void CollectEntityReferences(const json &scene, RequiredAssets &req) {
        const auto entities = scene.find("entities");
        if (entities == scene.end() || !entities->is_array())
            return;

        for (const auto &entity : *entities) {
            if (!entity.is_object())
                continue;

            const auto components = entity.find("components");
            if (components == entity.end() || !components->is_object())
                continue;

            if (const auto component = components->find("MeshComponent"); component != components->end()) {
                AddField(req.meshes, *component, "mesh_id");
            }

            if (const auto component = components->find("MaterialComponent"); component != components->end()) {
                AddField(req.materials, *component, "material_id");
            }

            if (const auto component = components->find("DirectionalTextureComponent"); component != components->end()) {
                AddStrArray(req.textures, *component, "textures");
            }

            if (const auto component = components->find("ParticleEmitterComponent"); component != components->end()) {
                AddField(req.materials, *component, "material_id");
            }

            if (const auto component = components->find("SpriteSheetComponent"); component != components->end()) {
                AddField(req.textures, *component, "texture_id");
            }

            if (const auto component = components->find("SpriteAnimatorComponent"); component != components->end()) {
                AddField(req.animationSets, *component, "animation_id");
            }
        }
    }

    RequiredAssets CollectDirectRefs(const json &scene) {
        RequiredAssets req;
        CollectGridReferences(scene, req);
        CollectPostProcReferences(scene, req);
        CollectEntityReferences(scene, req);
        CollectUIReferences(scene, req);
        return req;
    }

} // namespace


namespace IO::SceneAssetLoader {

    struct AssetRef {
        std::string type;
        std::string id;
        bool operator==(const AssetRef &) const = default;
    };

    struct AssetRefHash {
        std::size_t operator()(const AssetRef &ref) const {
            const auto typeHash = std::hash<std::string>{}(ref.type);
            const auto idHash = std::hash<std::string>{}(ref.id);
            return typeHash ^ (idHash << 1);
        }
    };

    using AssetSet = std::unordered_set<AssetRef, AssetRefHash>;

    void Add(AssetSet &assets, std::string type, const std::string &id) {
        if (id.empty() || id.starts_with("[Engine]")) {
            return;
        }
        assets.insert({std::move(type), id});
    }

    bool LoadReferences(const nlohmann::json &sceneData) { auto &resources = Core::ResourceManager::GetInstance(); }
} // namespace IO::SceneAssetLoader
