#pragma once

#include <cstdint>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string_view>

namespace IO::SceneAssetLoader {

    enum class AssetKind : std::uint8_t { Texture, Shader, Mesh, Material, Font, AnimationSet };

    class SceneAssetScope;

    bool LoadReferenced(const nlohmann::json &sceneData, SceneAssetScope &scope);
    bool Acquire(AssetKind kind, std::string_view id, SceneAssetScope &scope);

    class SceneAssetScope {
    public:
        SceneAssetScope();
        ~SceneAssetScope();

        SceneAssetScope(const SceneAssetScope &) = delete;
        SceneAssetScope &operator=(const SceneAssetScope &) = delete;

        SceneAssetScope(SceneAssetScope &&) noexcept;
        SceneAssetScope &operator=(SceneAssetScope &&) noexcept;

    private:
        struct State;
        std::unique_ptr<State> m_State;

        friend bool LoadReferenced(const nlohmann::json &sceneData, SceneAssetScope &scope);
        friend bool Acquire(AssetKind kind, std::string_view id, SceneAssetScope &scope);
    };

} // namespace IO::SceneAssetLoader
