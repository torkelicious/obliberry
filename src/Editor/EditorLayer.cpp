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
#include <Scripting/ObSLCore/ScriptRuntime.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <algorithm>

#include "Core/InputManager.h"
#include "Core/Window.h"
#include "FileDialogs.h"
#include "IO/SceneSerialization.h"
#include "IO/VFS.h"

/* TODO:
 * implement proper playmode / editmode / mapeditmode or something, enum exists but is unused
 * gizmos and shit
 * real editing ig
 * map editor etc etc just an actual editor
 * -
 * fix mouse offset when running via editor view - kinda done in a hacky way??
 */

bool Editor::EditorLayer::s_ShouldBuildDock = true;

void Editor::EditorLayer::Init(Core::EngineContext &ctx) {
    m_Context = ctx;

    m_Context.camera = &m_Camera;
    m_Context.sceneManager = &m_SceneManager;

    ctx.camera = &m_Camera;
    ctx.sceneManager = &m_SceneManager;

    m_SceneManager.SetContext(m_Context);

    m_Input = m_Context.input;
    m_Context.scriptPool->set_stdout(m_InterpreterOutput);

    if (Core::Project::GetActive()) {
        LoadStartScene();
        if (!m_PendingSceneToLoad.empty()) {
            LoadScene(m_PendingSceneToLoad);
            m_PendingSceneToLoad.clear();
        }
    } else {
        std::cout << "[EditorLayer] No active project" << std::endl;
    }
}


void Editor::EditorLayer::Update(const float dt) {
    if (!m_PendingSceneToLoad.empty()) {
        LoadScene(m_PendingSceneToLoad);
        m_PendingSceneToLoad.clear();
    }

    if (!m_Scene || !m_Registry) return;

    // Update systems
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
    std::cout << "[LoadScene] Loading scene: " << path << std::endl;

    ClearCurrentProject();

    try {
        m_SceneManager.LoadSceneByPath(path);
        m_Scene = m_SceneManager.GetCurrentScene();

        if (m_Scene) {
            m_Registry = &m_Scene->GetRegistry();
            m_CurrentScenePath = path;
            m_Scene->GetContext().camera = &m_Camera;

            if (m_Context.renderer) {
                Rendering::Renderer::SetClearColor(m_Scene->GetProperties().BackgroundClearColor);
            }

            std::cout << "[LoadScene] Successfully loaded scene with "
                    << m_Scene->GetRegistry().GetLivingEntities().size() << " entities" << std::endl;
        } else {
            std::cerr << "[LoadScene] Failed to load scene: " << path << std::endl;
            m_Registry = nullptr;
            m_CurrentScenePath.clear();
        }
    } catch (const std::exception &e) {
        std::cerr << "[LoadScene] Exception while loading scene: " << e.what() << std::endl;
        m_Registry = nullptr;
        m_CurrentScenePath.clear();
    }
}

void Editor::EditorLayer::ClearCurrentProject() {
    m_SceneManager.ClearCurrentScene();
    m_Scene = nullptr;
    m_Registry = nullptr;
    m_CurrentScenePath.clear();

    m_RegistryPanel.SetSelectedEntity(ECS::Entity{});
    m_InspectorPanel.SetSelectedEntity(ECS::Entity{});
    m_ViewportPanel.ClearSelectedEntityID();
}

void Editor::EditorLayer::LoadProject(const std::string &projectFilePath) {
    ClearCurrentProject();

    IO::VFS::UnmountProject();

    Core::Project::Load(projectFilePath);

    LoadStartScene();
}

void Editor::EditorLayer::LoadStartScene() {
    if (!Core::Project::GetActive()) {
        std::cerr << "[LoadStartScene] No active project!" << std::endl;
        return;
    }

    const std::string startScene = Core::Project::GetActive()->GetConfig().startScenePath;
    if (startScene.empty()) {
        std::cerr << "[LoadStartScene] Warning: startScenePath is empty in project configuration!" << std::endl;
        return;
    }

    std::cout << "[LoadStartScene] Loading start scene: " << startScene << std::endl;
    m_PendingSceneToLoad = startScene;
}

void Editor::EditorLayer::SaveScene() const {
    // ReSharper disable once CppExpressionWithoutSideEffects
    static_cast<void>(m_SceneManager.SaveCurrentScene());
}

void Editor::EditorLayer::DrawInterface() {
    if (!Core::Project::GetActive()) {
        DrawProjectHub();
    } else {
        DrawDockSpace();
        DrawToolbar();
        DrawEditorPanels();
        DrawGameView();
        DrawUtilityWindows();
    }

    // encapsulated dialogs
    // todo: maybe not this?
    m_NewProjectDialog.Update();
    if (Core::Project::GetActive()) {
        m_CreateSceneDialog.Update();
        m_SaveSceneAsDialog.Update();
    }
}

void Editor::EditorLayer::DrawProjectHub() {
    const ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
    const auto hubSize = ImVec2(viewportSize.x * 0.4f, viewportSize.y * 0.5f);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(hubSize);
    ImGui::Begin("Obliberry hub", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    ImGui::Text("welcome :)");
    ImGui::Separator();

    if (ImGui::Button("Create New Project", ImVec2(250, 50))) {
        const auto dir = FileDialogs::PickFolder(m_Context);
        if (dir) {
            m_NewProjectDialog.SetDirectory(std::filesystem::path(*dir));
            m_NewProjectDialog.SetOnConfirm([this](const std::filesystem::path &pDir, const std::string &name) {
                const auto newProject = Core::Project::NewProject(pDir, name);
                if (newProject) {
                    LoadProject(newProject->GetProjectPath().string());
                }
            });
            m_NewProjectDialog.Open();
        }
    }

    ImGui::Spacing();

    if (ImGui::Button("Open Existing Project", ImVec2(250, 50))) {
        const auto dir = FileDialogs::PickFolder(m_Context);
        if (dir) {
            const std::filesystem::path projectFile = std::filesystem::path(*dir) / "project.json";
            if (std::filesystem::exists(projectFile)) {
                LoadProject(projectFile.string());
            } else {
                std::cerr << "[Hub] No project.json found in: " << *dir << "\n";
            }
        }
    }

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
        const auto eID = static_cast<ECS::EntityID>(clickedID);

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
                ImGui::Image(texId, gameViewportSize, ImVec2{0, 1}, ImVec2{1, 0});
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
            if (ImGui::MenuItem("New Project")) {
                const auto dir = FileDialogs::PickFolder(m_Context);
                if (dir) {
                    m_NewProjectDialog.SetDirectory(std::filesystem::path(*dir));
                    m_NewProjectDialog.SetOnConfirm([this](const std::filesystem::path &pDir, const std::string &name) {
                        const auto newProject = Core::Project::NewProject(pDir, name);
                        if (newProject) {
                            LoadProject(newProject->GetProjectPath().string());
                        }
                    });
                    m_NewProjectDialog.Open();
                }
            }

            if (ImGui::MenuItem("Open Project")) {
                const auto dir = FileDialogs::PickFolder(m_Context);
                if (dir) {
                    const std::filesystem::path projectFile = std::filesystem::path(*dir) / "project.json";
                    if (std::filesystem::exists(projectFile)) {
                        LoadProject(projectFile.string());
                    } else {
                        std::cerr << "[Editor] No project.json found in: " << *dir << "\n";
                    }
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                SaveScene();
            }

            if (ImGui::MenuItem("Save Scene As...")) {
                std::string currentName = "scene";
                if (m_Scene && !m_Scene->GetProperties().Name.empty()) {
                    currentName = m_Scene->GetProperties().Name;
                }

                m_SaveSceneAsDialog.SetCurrentName(currentName);
                m_SaveSceneAsDialog.SetOnConfirm([this](const std::string &newName) {
                    std::string safeName = newName;
                    std::ranges::replace(safeName, ' ', '_');
                    const std::string scenePath = Core::PathUtils::Join(Core::SCENE_PATH, safeName, ".json");
                    if (IO::SceneIO::Serialize(scenePath, *m_Scene)) {
                        m_Scene->GetProperties().ScenePath = scenePath;
                        m_Scene->GetProperties().Name = newName;
                    }
                });
                m_SaveSceneAsDialog.Open();
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Close Project")) {
                ClearCurrentProject();
                Core::Project::SetActive(nullptr);
                IO::VFS::UnmountProject();
                s_ShouldBuildDock = true;
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) {
                m_Context.window->Close();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scene")) {
            if (ImGui::MenuItem("Create Scene")) {
                m_CreateSceneDialog.Reset();
                m_CreateSceneDialog.SetOnConfirm([this](const std::string &sceneName) {
                    // ReSharper disable once CppExpressionWithoutSideEffects
                    static_cast<void>(m_SceneManager.CreateNewScene(sceneName));
                });
                m_CreateSceneDialog.Open();
            }
            ImGui::Separator();

            if (ImGui::BeginMenu("Switch To")) {
                const auto scenes = m_SceneManager.GetAvailableScenes();
                if (scenes.empty()) {
                    ImGui::MenuItem("(no scenes)", nullptr, false, false);
                } else {
                    for (const auto &scenePath: scenes) {
                        const bool isCurrent = m_Scene && m_Scene->GetScenePath() == scenePath;
                        if (ImGui::MenuItem(scenePath.c_str(), nullptr, false, !isCurrent)) {
                            m_PendingSceneToLoad = scenePath;
                        }
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        // Play/Stop button centered
        const float buttonWidth = 60.0f;
        const float centerPos = ImGui::GetWindowSize().x * 0.5f - buttonWidth * 0.5f;

        ImGui::SetCursorPosX(centerPos);

        if (ImGui::Button(m_Playing ? "Stop" : "Play", ImVec2(buttonWidth, 0))) {
            m_Playing = !m_Playing;

            if (m_Playing) {
                std::cout << "[Editor] Entering Play Mode\n";
                // TODO: Initialize a real play mode
            } else {
                std::cout << "[Editor] Returning to Edit Mode\n";
                // TODO: Restore scene state
            }
        }
        ImGui::EndMainMenuBar();
    }
}
