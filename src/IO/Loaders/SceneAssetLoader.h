#pragma once

#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace IO::SceneAssetLoader {
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
    };

    bool LoadReferenced(const nlohmann::json &sceneData, SceneAssetScope &scope);
} // namespace IO::SceneAssetLoader
