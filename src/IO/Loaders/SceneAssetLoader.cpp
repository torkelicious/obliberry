#include "SceneAssetLoader.h"

#include "Core/ResourceManager.h"
#include "IO/AssetCatalog.h"
#include "IO/Loaders/AssetLoader.h"
#include "IO/VFS/VFS.h"
#include "Logger/LoggerService.h"
#include "nlohmann/json.hpp"
#include <set>
#include <string>

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

    bool ResolveDependencies(RequiredAssets &req) {

        for (const std::string &material_id : req.materials) {
            const json *mat = IO::AssetCatalog::Find("materials", material_id);
            if (!mat) {
                LOG_ERROR("SceneAssetLoader", "Material '" + material_id + "' is missing from assets.json");
                return false;
            }
            AddField(req.shaders, *mat, "shader");
            AddField(req.textures, *mat, "texture");
        }

        // animations need texture !
        for (const std::string &anim_id : req.animationSets) {
            const json *anim = IO::AssetCatalog::Find("animation_sets", anim_id);

            if (!anim) {
                LOG_ERROR("SceneAssetLoader", "Animation set '" + anim_id + "' is missing from assets.json");
                return false;
            }
            const auto path = anim->find("path");
            if (path == anim->end() || !path->is_string()) {
                LOG_ERROR("SceneAssetLoader", "Animation set '" + anim_id + "' has no valid path");
                return false;
            }
            const auto animationJson = IO::VFS::ReadVirtualJson(path->get<std::string>());
            if (!animationJson) {
                LOG_ERROR("SceneAssetLoader", "Could not read animation set '" + anim_id + "'");
                return false;
            }
            const auto sheet = animationJson->find("sheet");
            if (sheet != animationJson->end() && sheet->is_object()) {
                AddField(req.textures, *sheet, "texture_id");
            }
        }
        return true;
    }

    bool AppendAssets(json &destination, const char *type, const std::set<std::string> &ids) {
        destination[type] = json::array();
        for (const auto &id : ids) {
            const json *asset = IO::AssetCatalog::Find(type, id);
            if (!asset) {
                LOG_ERROR("SceneAssetLoader", "Could not find: " + id + " in catalog");
                return false;
            }
            destination[type].push_back(*asset);
        }
        return true;
    }

    bool BuildAssetSubset(const RequiredAssets &req, json &sub) {
        sub = json::object();
        return AppendAssets(sub, "textures", req.textures) && AppendAssets(sub, "shaders", req.shaders) && AppendAssets(sub, "meshes", req.meshes) && AppendAssets(sub, "materials", req.materials) &&
               AppendAssets(sub, "fonts", req.fonts) && AppendAssets(sub, "animation_sets", req.animationSets);
    }

} // namespace

namespace IO::SceneAssetLoader {
    bool LoadReferenced(const nlohmann::json &sceneData) {
        auto &resources = Core::ResourceManager::GetInstance();
        RequiredAssets required = CollectDirectRefs(sceneData);
        if (!ResolveDependencies(required)) {
            return false;
        }
        json subset;
        if (!BuildAssetSubset(required, subset)) {
            return false;
        }
        AssetLoader::LoadAssets(subset, resources);
        return true;
    }
} // namespace IO::SceneAssetLoader
