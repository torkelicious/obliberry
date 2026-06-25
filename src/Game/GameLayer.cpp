#include "GameLayer.h"
#include "IO/AssetLoader.h"
#include "Renderer/MeshFactory.h"
#include <filesystem>
#include "Core/ProjectConfig.h"
#include "Core/Window.h"
#include "Renderer/Renderer.h"
#include "Scripting/EngineLib/EngineLib.h"

void GameLayer::Init(EngineContext &ctx) {
    m_Context = ctx;

    m_Context.sceneManager = &m_SceneManager;
    m_SceneManager.LoadScene(
        std::make_unique<Scene>(
            m_Context,
            SceneProperties{.ScenePath = m_Context.projectConfig ? m_Context.projectConfig->startScenePath : ""}
        )
    );
}

void GameLayer::Update(const float dt) {
    m_Context.deltaTime = dt;

    if (!m_Context.pendingScenePath.empty()) {
        const std::string nextScene = std::move(m_Context.pendingScenePath);
        m_Context.pendingScenePath.clear();

        m_SceneManager.LoadScene(
            std::make_unique<Scene>(m_Context, SceneProperties{.ScenePath = nextScene}));
    }

    DrawInterface();
    if (m_GameIsRunning) {
        m_SceneManager.Update(dt);
    }
}

void GameLayer::Render() {
    if (m_Context.camera && m_Context.window) {
        const float aspect = static_cast<float>(m_Context.window->GetWidth()) /
                             static_cast<float>(m_Context.window->GetHeight());
        m_Context.renderer->SetCamera(*m_Context.camera, aspect);
    }
    m_SceneManager.Render();
}

void GameLayer::Shutdown() {
}
