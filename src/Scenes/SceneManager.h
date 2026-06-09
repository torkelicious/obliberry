#ifndef OBLIBERRY_SCENEMANAGER_H
#define OBLIBERRY_SCENEMANAGER_H
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

    void Update(float dt) {
        if (m_CurrentScene) {
            m_CurrentScene->Update(dt);
        }
    }

    void Render() {
        if (m_CurrentScene) {
            m_CurrentScene->Render();
        }
    }

private:
    std::unique_ptr<Scene> m_CurrentScene = nullptr;
};

#endif //OBLIBERRY_SCENEMANAGER_H
