#include "EditorLayer.h"

#include <iostream>

#include "Core/ProjectConfig.h"
#include "Core/Window.h"
#include "ECS/Systems/LightingSystem.h"

void EditorLayer::Init(EngineContext& ctx) {
    m_Context = ctx;
    m_Camera = m_Context.camera;
    m_Context.sceneManager = &m_SceneManager;
    m_Input = m_Context.input;

    m_SceneManager.LoadScene(std::make_unique<Scene>(
        m_Context,
        SceneProperties{.ScenePath = m_Context.projectConfig ? m_Context.projectConfig->startScenePath : ""}));

    m_Scene = m_SceneManager.GetCurrentScene();
    m_Registry = &m_Scene->GetRegistry();
}

void EditorLayer::Update(float dt) {
    if (m_Playing) {
        m_SceneManager.Update(dt);
    } else {
        LightingSystem::Update(*m_Registry);
    }
    HandleInput(dt);
}

void EditorLayer::Render() {
    if (m_Context.camera && m_Context.window) {
        const float aspect =
            static_cast<float>(m_Context.window->GetWidth()) / static_cast<float>(m_Context.window->GetHeight());
        m_Context.renderer->SetCamera(*m_Context.camera, aspect);
    }

    m_SceneManager.Render();
}

void EditorLayer::Shutdown() { ApplicationLayer::Shutdown(); }

void EditorLayer::DrawInterface() {}

void EditorLayer::HandleInput(float dt) {
    if (m_Input != nullptr) {
        if (m_Input->IsKeyDown("Esc")) {
            m_Context.window->Close();
        }
    }
}

void EditorLayer::LoadScene(const std::string& path) {}

void EditorLayer::SaveScene() {}
