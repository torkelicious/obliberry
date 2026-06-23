#pragma once


#include <memory>
#include "Scene.h"

class SceneManager {
public:
    void LoadScene(std::unique_ptr<Scene> newScene) {
        if (m_CurrentScene) {
            m_CurrentScene->OnExit();
        }
        m_CurrentScene = std::move(newScene);
        if (m_CurrentScene) {
            m_CurrentScene->OnEnter();
        }
    }

    void Update(const float dt) const {
        if (m_CurrentScene) {
            m_CurrentScene->Update(dt);
        }
    }

    void Render() const {
        if (m_CurrentScene) {
            m_CurrentScene->Render();
        }
    }

    [[nodiscard]] Scene *GetCurrentScene() const { return m_CurrentScene.get(); }

private:
    std::unique_ptr<Scene> m_CurrentScene = nullptr;
};

