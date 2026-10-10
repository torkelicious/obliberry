#include "SceneAssetLoader.h"
#include <algorithm>

#include "Core/ResourceManager.h"
#include "ECS/Systems/Animation/Types.h"
#include "IO/Loaders/AssetLoader.h"
#include "IO/AssetDependencies.h"
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
    using IO::AssetDependencies::BuildAssetSubset;
    using IO::AssetDependencies::CollectDirectRefs;
    using IO::AssetDependencies::IsCatalogAsset;
    using IO::AssetDependencies::RequiredAssets;
    using IO::AssetDependencies::ResolveDependencies;
    using IO::SceneAssetLoader::AssetKind;

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
    std::unordered_set<AssetKey, AssetKeyHash> s_StaleAssets;

    std::optional<AssetKind> KindFromCatalogType(const std::string_view type) {
        if (type == "textures")
            return AssetKind::Texture;
        if (type == "shaders")
            return AssetKind::Shader;
        if (type == "meshes")
            return AssetKind::Mesh;
        if (type == "materials")
            return AssetKind::Material;
        if (type == "fonts")
            return AssetKind::Font;
        if (type == "animation_sets")
            return AssetKind::AnimationSet;

        return std::nullopt;
    }


    template <typename Func> void ForEachAsset(const RequiredAssets &assets, Func &&func) {
        for (const auto &id : assets.textures)
            func(AssetKey{.kind = AssetKind::Texture, .id = id});
        for (const auto &id : assets.shaders)
            func(AssetKey{.kind = AssetKind::Shader, .id = id});
        for (const auto &id : assets.meshes)
            func(AssetKey{.kind = AssetKind::Mesh, .id = id});
        for (const auto &id : assets.materials)
            func(AssetKey{.kind = AssetKind::Material, .id = id});
        for (const auto &id : assets.fonts)
            func(AssetKey{.kind = AssetKind::Font, .id = id});
        for (const auto &id : assets.animationSets)
            func(AssetKey{.kind = AssetKind::AnimationSet, .id = id});
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

    bool AddRequiredAsset(RequiredAssets &req, const AssetKind kind, const std::string &id) {
        if (id.empty()) {
            return false;
        }

        switch (kind) {
            case AssetKind::Texture:
                req.textures.insert(id);
                return true;
            case AssetKind::Shader:
                req.shaders.insert(id);
                return true;
            case AssetKind::Mesh:
                req.meshes.insert(id);
                return true;
            case AssetKind::Material:
                req.materials.insert(id);
                return true;
            case AssetKind::Font:
                req.fonts.insert(id);
                return true;
            case AssetKind::AnimationSet:
                req.animationSets.insert(id);
                return true;
        }
        return false;
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

    void SceneAssetScope::Merge(SceneAssetScope scope) {
        if (!scope.m_State) {
            return;
        }

        if (!m_State) {
            m_State = std::move(scope.m_State);
            return;
        }

        auto &owned = m_State->assets;
        auto &incoming = scope.m_State->assets;

        for (auto it = incoming.begin(); it != incoming.end();) {
            if (std::ranges::find(owned, *it) != owned.end()) {
                ++it;
                continue;
            }

            owned.push_back(std::move(*it));
            it = incoming.erase(it);
        }
    }

    std::size_t SceneAssetScope::GetAssetCount() const noexcept { return m_State ? m_State->assets.size() : 0; }

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

    bool Acquire(const AssetKind kind, const std::string_view id, SceneAssetScope &scope) {
        scope = SceneAssetScope{};

        if (id.empty()) {
            return false;
        }

        const std::string assetId(id);
        const AssetKey key{.kind = kind, .id = assetId};

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

    void MarkStale(const std::string_view catalogType, const std::string_view id) {
        if (id.empty()) {
            return;
        }
        const auto kind = KindFromCatalogType(catalogType);
        if (!kind) {
            return;
        }
        std::lock_guard lock(s_ResidencyMutex);
        s_StaleAssets.insert(AssetKey{.kind = *kind, .id = std::string(id)});
    }

    void UnloadStale() {
        std::lock_guard lock(s_ResidencyMutex);
        for (const auto &asset : s_StaleAssets) {
            Unload(asset);
            s_ReferenceCounts.erase(asset);
        }
        s_StaleAssets.clear();
    }


} // namespace IO::SceneAssetLoader
