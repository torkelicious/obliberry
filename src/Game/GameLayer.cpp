#include "GameLayer.h"
#include "IO/AssetLoader.h"
#include "Rendering/MeshFactory.h"
#include <filesystem>
#include "Core/ProjectConfig.h"
#include "Core/Window.h"
#include "Rendering/Renderer.h"
#include "Scripting/EngineLib/EngineLib.h"

void Game::GameLayer::Init(Core::EngineContext &ctx) {
    m_Context = &ctx;

    m_Context->sceneManager = &m_SceneManager;

    m_SceneManager.SetContext(*m_Context);

    std::string startScene;
    if (m_Context->projectConfig) {
        startScene = m_Context->projectConfig->startScenePath;
    }

    if (!startScene.empty()) {
        m_SceneManager.LoadSceneByPath(startScene);
    }
}

void Game::GameLayer::Update(const float dt) {
    if (!m_Context) return;

    m_Context->deltaTime = dt;

    m_SceneManager.ProcessPendingSceneChange(*m_Context);

    DrawInterface();
    if (m_GameIsRunning) {
        m_SceneManager.Update(dt);
    }
}

void Game::GameLayer::Render() {
    if (!m_Context) return;
    if (m_Context->camera && m_Context->window) {
        const float aspect = static_cast<float>(m_Context->window->GetWidth()) /
                             static_cast<float>(m_Context->window->GetHeight());
        m_Context->renderer->SetCamera(*m_Context->camera, aspect);
    }
    m_SceneManager.Render();
}

void Game::GameLayer::Shutdown() {
}
