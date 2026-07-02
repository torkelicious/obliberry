#pragma once


#include <memory>
#include <vector>
#include "Core/EngineContext.h"
#include "Scene.h"

namespace Scenes {
    class SceneManager {
    public:
        [[nodiscard]] std::vector<std::string> GetAvailableScenes() const;

        [[nodiscard]] bool CreateNewScene(const std::string &sceneName) const;

        bool DeleteScene(const std::string &scenePath);

        [[nodiscard]] std::string GetCurrentScenePath() const;

        void SwitchScene(const std::string &newScenePath);

        void LoadSceneByPath(const std::string &scenePath);

        [[nodiscard]] bool SaveCurrentScene() const;

        [[nodiscard]] bool ValidateScenePath(const std::string &scenePath) const;

        void ProcessPendingSceneChange(Core::EngineContext &context);

        void ClearCurrentScene();

        void LoadScene(std::unique_ptr<Scene> newScene);

        void Update(float dt) const;

        void Render() const;

        [[nodiscard]] Scene *GetCurrentScene() const { return m_CurrentScene.get(); }

        void SetContext(Core::EngineContext &ctx) {
            m_Context = &ctx;
        }

    private:
        Core::EngineContext *m_Context = nullptr;
        std::unique_ptr<Scene> m_CurrentScene = nullptr;
    };
} // namespace Scenes
