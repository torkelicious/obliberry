#include "SceneAssetLoader.h"

#include "Core/ResourceManager.h"
#include "ECS/Systems/Animation/Types.h"
#include "IO/AssetCatalog.h"
#include "IO/Loaders/AssetLoader.h"
#include "IO/VFS/VFS.h"
#include "Logger/LoggerService.h"
#include "Rendering/Types/Material.h"
#include "Rendering/Types/Mesh/Mesh.h"
#include "Rendering/Types/Shader/Shader.h"
#include "Rendering/Types/Texture/Texture.h"
#include "UI/Text/Font.h"
#include "nlohmann/json.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <optional>
#include <string_view>

namespace {
    using json = nlohmann::json;
    using IO::SceneAssetLoader::AssetKind;

    struct RequiredAssets {
        std::set<std::string> textures;
        std::set<std::string> shaders;
        std::set<std::string> meshes;
        std::set<std::string> materials;
        std::set<std::string> fonts;
        std::set<std::string> animationSets;
    };

    struct AssetKey {
        AssetKind kind;
        std::string id;

        bool operator==(const AssetKey &) const = default;
    };

    struct AssetKeyHash {
        std::size_t operator()(const AssetKey &key) const noexcept { return std::hash<std::string>{}(key.id) ^ (static_cast<std::size_t>(key.kind) << 1); }
    };

    std::mutex s_ResidencyMutex;
    std::unordered_map<AssetKey, std::size_t, AssetKeyHash> s_ReferenceCounts;

    template <typename Func> void ForEachAsset(const RequiredAssets &assets, Func &&func) {
        for (const auto &id : assets.textures)
            func(AssetKey{AssetKind::Texture, id});
        for (const auto &id : assets.shaders)
            func(AssetKey{AssetKind::Shader, id});
        for (const auto &id : assets.meshes)
            func(AssetKey{AssetKind::Mesh, id});
        for (const auto &id : assets.materials)
            func(AssetKey{AssetKind::Material, id});
        for (const auto &id : assets.fonts)
            func(AssetKey{AssetKind::Font, id});
        for (const auto &id : assets.animationSets)
            func(AssetKey{AssetKind::AnimationSet, id});
    }

    bool IsLoaded(const AssetKey &asset) {
        auto &resources = Core::ResourceManager::GetInstance();
        switch (asset.kind) {
            case AssetKind::Texture:
                return resources.Get<Rendering::Texture>(asset.id) != nullptr;
            case AssetKind::Shader:
                return resources.Get<Rendering::Shader>(asset.id) != nullptr;
            case AssetKind::Mesh:
                return resources.Get<Rendering::Mesh>(asset.id) != nullptr;
            case AssetKind::Material:
                return resources.Get<Rendering::Material>(asset.id) != nullptr;
            case AssetKind::Font:
                return resources.Get<UI::Font>(asset.id) != nullptr;
            case AssetKind::AnimationSet:
                return resources.Get<Animation::SpriteAnimationSet>(asset.id) != nullptr;
        }
        return false;
    }

    void Unload(const AssetKey &asset) {
        auto &resources = Core::ResourceManager::GetInstance();
        switch (asset.kind) {
            case AssetKind::Texture:
                resources.Unload<Rendering::Texture>(asset.id);
                break;
            case AssetKind::Shader:
                resources.Unload<Rendering::Shader>(asset.id);
                break;
            case AssetKind::Mesh:
                resources.Unload<Rendering::Mesh>(asset.id);
                break;
            case AssetKind::Material:
                resources.Unload<Rendering::Material>(asset.id);
                break;
            case AssetKind::Font:
                resources.Unload<UI::Font>(asset.id);
                break;
            case AssetKind::AnimationSet:
                resources.Unload<Animation::SpriteAnimationSet>(asset.id);
                break;
        }
    }

    void ReleaseAssets(const std::vector<AssetKey> &assets) {
        std::lock_guard lock(s_ResidencyMutex);

        for (auto it = assets.rbegin(); it != assets.rend(); ++it) {
            const auto count = s_ReferenceCounts.find(*it);
            if (count == s_ReferenceCounts.end())
                continue;

            if (--count->second == 0) {
                Unload(*it);
                s_ReferenceCounts.erase(count);
            }
        }
    }


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
            if (type.is_object() && !type.contains("texture")) {
                req.textures.insert("hex_tex");
            } else {
                AddField(req.textures, type, "texture");
            }
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

    bool AddRequiredAsset(RequiredAssets &req, const AssetKind kind, const std::string id) {
        if (id.empty()) {
            return false;
        }

        switch (kind) {
            case IO::SceneAssetLoader::AssetKind::Texture:
                req.textures.insert(id);
                return true;
            case IO::SceneAssetLoader::AssetKind::Shader:
                req.shaders.insert(id);
                return true;
            case IO::SceneAssetLoader::AssetKind::Mesh:
                req.meshes.insert(id);
                return true;
            case IO::SceneAssetLoader::AssetKind::Material:
                req.materials.insert(id);
                return true;
            case IO::SceneAssetLoader::AssetKind::Font:
                req.fonts.insert(id);
                return true;
            case IO::SceneAssetLoader::AssetKind::AnimationSet:
                req.animationSets.insert(id);
                return true;
        }
        return false;
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

    std::optional<std::vector<AssetKey>> AcquireRequiredAssets(RequiredAssets req) {
        if (!ResolveDependencies(req)) {
            return std::nullopt;
        }

        json subset;
        if (!BuildAssetSubset(req, subset)) {
            return std::nullopt;
        }

        auto &resources = Core::ResourceManager::GetInstance();

        std::lock_guard residencyLock(s_ResidencyMutex);
        std::unordered_set<AssetKey, AssetKeyHash> alreadyLoaded;

        ForEachAsset(req, [&](const AssetKey &asset) {
            if (IsLoaded(asset)) {
                alreadyLoaded.insert(asset);
            }
        });

        IO::AssetLoader::LoadAssets(subset, resources);
        bool allLoaded = true;

        ForEachAsset(req, [&](const AssetKey &asset) {
            if (!IsLoaded(asset)) {
                LOG_ERROR("SceneAssetLoader", "Asset failed to load: " + asset.id);

                allLoaded = false;
            }
        });

        if (!allLoaded) {
            std::vector<AssetKey> loadedThisAttempt;

            ForEachAsset(req, [&](const AssetKey &asset) {
                if (!alreadyLoaded.contains(asset) && IsLoaded(asset))
                    loadedThisAttempt.push_back(asset);
            });

            for (auto it = loadedThisAttempt.rbegin(); it != loadedThisAttempt.rend(); ++it) {
                Unload(*it);
            }
            return std::nullopt;
        }

        std::vector<AssetKey> acquiredAssets;

        ForEachAsset(req, [&](const AssetKey &asset) {
            const auto existing = s_ReferenceCounts.find(asset);
            if (existing != s_ReferenceCounts.end()) {
                ++existing->second;
                acquiredAssets.push_back(asset);
                return;
            }

            if (alreadyLoaded.contains(asset)) { // treat as pinned
                return;
            }

            s_ReferenceCounts.emplace(asset, 1);
            acquiredAssets.push_back(asset);
        });

        return acquiredAssets;
    }

} // namespace

namespace IO::SceneAssetLoader {
    struct SceneAssetScope::State {
        std::vector<AssetKey> assets;

        ~State() { ReleaseAssets(assets); }
    };

    SceneAssetScope::SceneAssetScope() = default;
    SceneAssetScope::~SceneAssetScope() = default;
    SceneAssetScope::SceneAssetScope(SceneAssetScope &&) noexcept = default;
    SceneAssetScope &SceneAssetScope::operator=(SceneAssetScope &&) noexcept = default;

    bool LoadReferenced(const nlohmann::json &sceneData, SceneAssetScope &scope) {
        scope = SceneAssetScope{};

        auto acquired = AcquireRequiredAssets(CollectDirectRefs(sceneData));

        if (!acquired)
            return false;

        auto state = std::make_unique<SceneAssetScope::State>();
        state->assets = std::move(*acquired);
        scope.m_State = std::move(state);

        return true;
    }

    bool Acquire(AssetKind kind, const std::string_view id, SceneAssetScope &scope) {
        scope = SceneAssetScope{};

        if (id.empty()) {
            return false;
        }

        const std::string assetId(id);
        const AssetKey key{kind, assetId};

        // permanent objs
        if (!IsCatalogAsset(assetId)) {
            return IsLoaded(key);
        }

        RequiredAssets required;
        if (!AddRequiredAsset(required, kind, assetId)) {
            return false;
        }

        auto acquired = AcquireRequiredAssets(std::move(required));

        if (!acquired) {
            return false;
        }

        auto state = std::make_unique<SceneAssetScope::State>();
        state->assets = std::move(*acquired);
        scope.m_State = std::move(state);
        return true;
    }

} // namespace IO::SceneAssetLoader
