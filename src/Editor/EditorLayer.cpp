#include "EditorLayer.h"

#include "Core/ProjectConfig.h"
#include "ECS/ECS.h"
#include "ECS/Entity.h"
#include "ECS/Systems/LightingSystem.h"
#include "Scenes/Scene.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdint>
#include <ImGuizmo.h>
#include <memory>

#include "Core/InputManager.h"
#include "Core/Window.h"

bool EditorLayer::s_ShouldBuildDock = true;

void EditorLayer::Init(EngineContext &ctx) {
    m_Context = ctx;
    m_Context.camera = &m_Camera;
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
    ImGuizmo::BeginFrame();
    DrawInterface();

    // camera aspect ratio based on the panel
    if (m_Context.camera) {
        const float aspect = m_ViewportPanel.GetViewportWidth() / m_ViewportPanel.GetViewportHeight();
        m_Context.renderer->SetCamera(*m_Context.camera, aspect);
    }

    m_SceneManager.Render();
}

void EditorLayer::Shutdown() {
}

void EditorLayer::HandleInput(float dt) {
    if (m_Input->IsKeyPressed("Esc")) {
        m_Context.window->Close();
    }

    if (m_Input->IsKeyPressed("V")) {
        m_Camera.ToggleViewMode();
    }

    if (!m_ViewportPanel.IsHovered()) return;

    const float scrollDelta = static_cast<float>(m_Input->ScrollY());
    if (scrollDelta != 0.0f) {
        const float zoomSens = 0.2f;
        m_Camera.AdjustZoom(scrollDelta * zoomSens);
    }

    const float mouseDeltaX = static_cast<float>(m_Input->GetMouseDeltaX());
    const float mouseDeltaY = static_cast<float>(m_Input->GetMouseDeltaY());

    if (m_Input->IsMouseDown("MouseMiddle") || m_Input->IsMouseDown("MouseRight")) {
        const float mousePanSens = 0.025f;
        m_Camera.Pan(-mouseDeltaX, mouseDeltaY, mousePanSens);
    }

    float kbPanX = 0.0f;
    float kbPanY = 0.0f;

    if (m_Input->IsKeyDown("W")) kbPanY += 1.0f;
    if (m_Input->IsKeyDown("S")) kbPanY -= 1.0f;
    if (m_Input->IsKeyDown("A")) kbPanX -= 1.0f;
    if (m_Input->IsKeyDown("D")) kbPanX += 1.0f;

    if (kbPanX != 0.0f || kbPanY != 0.0f) {
        const float length = std::sqrt(kbPanX * kbPanX + kbPanY * kbPanY);
        kbPanX /= length;
        kbPanY /= length;
        float speedMod = m_Input->IsKeyDown("LeftShift") ? 3.0f : 1.0f;
        const float kbPanSpeed = 15.0f * speedMod * dt;
        m_Camera.Pan(kbPanX, kbPanY, kbPanSpeed);
    }
}


void EditorLayer::LoadScene(const std::string &path) {
}

void EditorLayer::SaveScene() {
}

void EditorLayer::DrawInterface() {
    DrawDockSpace();
    DrawEditorPanels();
    DrawGameView();
    DrawUtilityWindows();
}

void EditorLayer::DrawDockSpace() {
    const ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");
    ImGui::DockSpaceOverViewport(dockspaceId, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    if (!s_ShouldBuildDock)
        return;

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId,
                              ImGuiDockNodeFlags_PassthruCentralNode |
                              static_cast<int>(ImGuiDockNodeFlags_DockSpace));
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

    ImGuiID dock_id_center = dockspaceId;
    const ImGuiID dock_id_right =
            ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Right, 0.25f, nullptr, &dock_id_center);
    const ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Left, 0.2f, nullptr,
                                                             &dock_id_center);
    const ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Down, 0.3f, nullptr,
                                                               &dock_id_center);

    ImGui::DockBuilderDockWindow("Registry", dock_id_left);
    ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
    ImGui::DockBuilderDockWindow("Console", dock_id_bottom);
    ImGui::DockBuilderDockWindow("Project Browser", dock_id_bottom);
    ImGui::DockBuilderDockWindow("Scene View", dock_id_center);
    ImGui::DockBuilderDockWindow("Game View", dock_id_center);

    ImGui::DockBuilderFinish(dockspaceId);
    s_ShouldBuildDock = false;
}

void EditorLayer::DrawEditorPanels() {
    m_RegistryPanel.SetContext(m_Scene, m_Context);
    m_InspectorPanel.SetContext(m_Scene, m_Context);
    m_ViewportPanel.SetContext(m_Scene, m_Context);

    m_InspectorPanel.SetSelectedEntity(m_RegistryPanel.GetSelectedEntity());

    m_RegistryPanel.OnImGuiRender();
    m_InspectorPanel.OnImGuiRender();
    m_ViewportPanel.OnImGuiRender();
}

void EditorLayer::DrawGameView() {
    //todo: implement
    ImGui::Begin("Game View");
    const ImVec2 gameViewportSize = ImGui::GetContentRegionAvail();

    if (!m_Playing) {
        if (gameViewportSize.x > 200.0f && gameViewportSize.y > 50.0f) {
            ImGui::SetCursorPos(ImVec2(gameViewportSize.x * 0.5f - 110.0f, gameViewportSize.y * 0.5f));
            ImGui::TextDisabled("Game is not running");
        }
    } else {
        if (gameViewportSize.x > 0.0f && gameViewportSize.y > 0.0f) {
            if (const auto fbo = m_Context.renderer->GetEditorFramebuffer()) {
                const uint32_t texId = fbo->GetColorAttID();
                ImGui::Image(reinterpret_cast<void *>(static_cast<intptr_t>(texId)), gameViewportSize, ImVec2{0, 1},
                             ImVec2{1, 0});
            }
        }
    }

    ImGui::End();
}

void EditorLayer::DrawUtilityWindows() {
    ImGui::Begin("Console");
    ImGui::End();

    ImGui::Begin("Project Browser");
    ImGui::End();
}
