#include "Game.h"
#include "IO/AssetLoader.h"
#include "Renderer/MeshFactory.h"
#include <filesystem>
#include "Core/ProjectConfig.h"
#include "Core/Window.h"
#include "Renderer/Renderer.h"
#include "Scripting/EngineLib/EngineLib.h"

void Game::Start() {
    MeshFactory::RegisterAllMeshFactories();

    m_Context.sceneManager = &m_SceneManager;
    m_SceneManager.LoadScene(
        std::make_unique<Scene>(
            m_Context,
            SceneProperties{.ScenePath = m_Context.projectConfig ? m_Context.projectConfig->startScenePath : ""}
        )
    );
}


void Game::Update(const float dt) {
    m_Context.deltaTime = dt;

    if (!m_Context.pendingScenePath.empty()) {
        const std::string nextScene = std::move(m_Context.pendingScenePath);
        m_Context.pendingScenePath.clear();

        m_SceneManager.LoadScene(
            std::make_unique<Scene>(m_Context, SceneProperties{.ScenePath = nextScene}));
    }

    // (Legacy/Fallback)
    if (m_PendingSceneLoad.has_value()) {
        m_SceneManager.LoadScene(
            std::make_unique<Scene>(m_Context, std::move(*m_PendingSceneLoad)));
        m_PendingSceneLoad.reset();
    }

    DrawInterface();
    if (m_CurrentState == GameState::Gameplay) {
        m_SceneManager.Update(dt);
    }
}

void Game::Render() const {
    if (m_Context.camera && m_Context.window) {
        const float aspect = static_cast<float>(m_Context.window->GetWidth()) /
                             static_cast<float>(m_Context.window->GetHeight());
        m_Context.renderer->SetCamera(*m_Context.camera, aspect);
    }

    m_Context.renderer->BeginFrame();
    m_SceneManager.Render();
}

void Game::Shutdown() {
}
