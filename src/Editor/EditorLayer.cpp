#include "EditorLayer.h"
#include "Core/Project.h"
#include "Core/ProjectConfig.h"
#include "ECS/ECS.h"
#include "ECS/Entity.h"
#include "ECS/Systems/LightingSystem.h"
#include "Scenes/Scene.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <ImGuizmo.h>
#include <Scripting/ObSLCore/Interpreter/Interpreter.h>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "FileDialogs.h"
#include "IO/VFS.h"
#include "Core/InputManager.h"
#include "Core/Window.h"

/* TODO:
 *  implement proper playmode / editmode / mapeditmode or something, enum exists but is unused
 *  gizmos and shit
 *  real editing ig
 *  map editor etc etc just an actual editor
 *  -
 *  fix mouse offset when running via editor view - kinda done in a hacky way??
 */

bool Editor::EditorLayer::s_ShouldBuildDock = true;

void Editor::EditorLayer::Init(Core::EngineContext &ctx) {
    m_Context = ctx;
    m_Context.camera = &m_Camera;
    m_Context.sceneManager = &m_SceneManager;
    m_Input = m_Context.input;
    m_Context.scriptEngine->Set_Stdout(m_InterpreterOutput);
    if (Core::Project::GetActive()) {
        const std::string startScene = Core::Project::GetActive()->GetConfig().startScenePath;
        m_SceneManager.LoadScene(
            std::make_unique<Scenes::Scene>(m_Context, Scenes::SceneProperties{.ScenePath = startScene}));

        m_Scene = m_SceneManager.GetCurrentScene();
        m_Registry = &m_Scene->GetRegistry();
    }
}

void Editor::EditorLayer::Update(const float dt) {
    if (Core::Project::GetActive() && !m_Scene) {
        const std::string startScene = Core::Project::GetActive()->GetConfig().startScenePath;

        m_SceneManager.LoadScene(
            std::make_unique<Scenes::Scene>(m_Context, Scenes::SceneProperties{.ScenePath = startScene}));

        m_Scene = m_SceneManager.GetCurrentScene();
        m_Registry = &m_Scene->GetRegistry();

        //m_Playing = true; // TEMP FOR TESTING
    }

    if (!m_Scene || !m_Registry) {
        return;
    }

    if (m_Playing) {
        m_SceneManager.Update(dt);
    } else {
        ECS::Systems::LightingSystem::Update(*m_Registry);
    }
    HandleInput(dt);
}

void Editor::EditorLayer::Render() {
    ImGuizmo::BeginFrame();
    DrawInterface();

    if (!Core::Project::GetActive())
        return;

    if (m_Context.camera) {
        const float aspect = m_ViewportPanel.GetWidth() / m_ViewportPanel.GetHeight();
        m_Context.renderer->SetCamera(*m_Context.camera, aspect);
    }

    m_SceneManager.Render();
}

void Editor::EditorLayer::Shutdown() {
}

void Editor::EditorLayer::HandleInput(const float dt) {
    if (m_Input->IsKeyPressed("Esc")) {
        m_Context.window->Close();
    }

    if (m_Input->IsKeyPressed("V")) {
        m_Camera.ToggleViewMode();
    }

    if (!m_ViewportPanel.IsHovered())
        return;

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

    if (m_Input->IsKeyDown("W"))
        kbPanY += 1.0f;
    if (m_Input->IsKeyDown("S"))
        kbPanY -= 1.0f;
    if (m_Input->IsKeyDown("A"))
        kbPanX -= 1.0f;
    if (m_Input->IsKeyDown("D"))
        kbPanX += 1.0f;

    if (kbPanX != 0.0f || kbPanY != 0.0f) {
        const float length = std::sqrt(kbPanX * kbPanX + kbPanY * kbPanY);
        kbPanX /= length;
        kbPanY /= length;
        const float speedMod = m_Input->IsKeyDown("LeftShift") ? 3.0f : 1.0f;
        const float kbPanSpeed = 15.0f * speedMod * dt;
        m_Camera.Pan(kbPanX, kbPanY, kbPanSpeed);
    }
}

void Editor::EditorLayer::LoadScene(const std::string &path) {
}

void Editor::EditorLayer::SaveScene() {
}

void Editor::EditorLayer::DrawInterface() {
    if (!Core::Project::GetActive()) {
        DrawProjectHub();
        return;
    }

    DrawDockSpace();
    DrawToolbar();
    DrawEditorPanels();
    DrawGameView();
    DrawUtilityWindows();
}

void Editor::EditorLayer::DrawProjectHub() {
    //TODO !!!:
    // Fix Deadlock in hub when trying to close resize or really do anything other than press the button!!!

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400));

    ImGui::Begin("Obliberry hub", nullptr,
                 ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    ImGui::Text("welcome :)");
    ImGui::Separator();

    if (ImGui::Button("Create New Project (TestGame)", ImVec2(250, 50))) {
        const std::filesystem::path homeDir = IO::VFS::GetHomeDirectory();
        const std::filesystem::path baseDir = homeDir / "OBLI_TEST_Projects";
        Core::Project::NewProject(baseDir, "TestGame");
    }

    // ImGui::SameLine();
    //
    // if (ImGui::Button("Open Existing Project", ImVec2(250, 50))) {
    //     std::cout << "not implemented\n";
    // }
    //

    ImGui::End();
}

void Editor::EditorLayer::DrawDockSpace() {
    const ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");
    ImGui::DockSpaceOverViewport(dockspaceId, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    if (!s_ShouldBuildDock)
        return;

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId,
                              ImGuiDockNodeFlags_PassthruCentralNode | static_cast<int>(ImGuiDockNodeFlags_DockSpace));
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

    ImGuiID dock_id_center = dockspaceId;

    const ImGuiID dock_id_top =
            ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Up, 0.06f, nullptr, &dock_id_center);

    const ImGuiID dock_id_right =
            ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Right, 0.25f, nullptr, &dock_id_center);
    const ImGuiID dock_id_left =
            ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Left, 0.2f, nullptr, &dock_id_center);
    const ImGuiID dock_id_bottom =
            ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Down, 0.3f, nullptr, &dock_id_center);

    ImGui::DockBuilderDockWindow("Registry", dock_id_left);
    ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
    ImGui::DockBuilderDockWindow("Console", dock_id_bottom);
    ImGui::DockBuilderDockWindow("Project Browser", dock_id_bottom);
    ImGui::DockBuilderDockWindow("Scene View", dock_id_center);
    ImGui::DockBuilderDockWindow("Game View", dock_id_center);

    ImGui::DockBuilderFinish(dockspaceId);
    s_ShouldBuildDock = false;
}


void Editor::EditorLayer::DrawEditorPanels() {
    m_RegistryPanel.SetContext(m_Scene, m_Context);
    m_InspectorPanel.SetContext(m_Scene, m_Context);
    m_ViewportPanel.SetContext(m_Scene, m_Context);

    const int clickedID = m_ViewportPanel.GetSelectedEntityID();

    if (clickedID != -1) {
        const ECS::EntityID eID = static_cast<ECS::EntityID>(clickedID);

        if (m_Registry->IsValid(eID)) {
            const ECS::Entity selectedEntity(eID, m_Registry);
            m_RegistryPanel.SetSelectedEntity(selectedEntity);
        }

        m_ViewportPanel.ClearSelectedEntityID();
    }

    m_InspectorPanel.SetSelectedEntity(m_RegistryPanel.GetSelectedEntity());

    m_RegistryPanel.OnImGuiRender();
    m_InspectorPanel.OnImGuiRender();
    m_ViewportPanel.OnImGuiRender();
}

void Editor::EditorLayer::DrawGameView() const {
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
                ImGui::Image(texId, gameViewportSize, ImVec2{0, 1},
                             ImVec2{1, 0});
            }
        }
    }
    ImGui::End();
}

void Editor::EditorLayer::DrawUtilityWindows() {
    FlushInterpreterOutput();

    ImGui::Begin("Console");

    if (ImGui::Button("Clear")) {
        m_ConsoleLogs.clear();
    }

    ImGui::Separator();

    ImGui::BeginChild("LogScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto &logLine: m_ConsoleLogs) {
        ImGui::TextUnformatted(logLine.c_str());
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();

    ImGui::Begin("Project Browser");
    ImGui::End();
}

void Editor::EditorLayer::DrawToolbar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Project")) {
                auto path = FileDialogs::PickFolder(m_Context);
                if (path.has_value()) {
                    //TODO: IMPLEMENT
                    std::cout << "[Editor] Picked Project Folder: " << path.value() << "\n";
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Scene As")) {
                auto path = FileDialogs::SaveFile(m_Context, "ObliBerry JSON Scene", "json");
                if (path.has_value()) {
                    //TODO: IMPLEMENT
                    std::cout << "[Editor] Saving Scene to: " << path.value() << "\n";
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                m_Context.window->Close();
            }
            ImGui::EndMenu();
        }

        // the center of the screen minus half the button's width
        float buttonWidth = 60.0f;
        float centerPos = (ImGui::GetWindowSize().x * 0.5f) - (buttonWidth * 0.5f);

        ImGui::SetCursorPosX(centerPos);

        if (ImGui::Button(m_Playing ? "Stop" : "Play", ImVec2(buttonWidth, 0))) {
            m_Playing = !m_Playing;

            if (m_Playing) {
                std::cout << "[Editor] Entering Play Mode\n";
                // TODO: Initialize play mode
            } else {
                std::cout << "[Editor] Returning to Edit Mode\n";
                // TODO: Restore scene state
            }
        }
        ImGui::EndMainMenuBar();
    }
}
