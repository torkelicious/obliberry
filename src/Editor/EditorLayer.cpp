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
#include <ObSL/ScriptRuntime.h>
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
#include "IO/Package/Tools/ObpakTools.h"

#include "States/EditState.h"
#include "States/PlayState.h"
#include "States/MapEditState.h"

/* TODO:
 * map editor
 * project browser (asset view ig)
 * whatever else an editor needs?
 */

bool Editor::EditorLayer::s_ShouldBuildDock = true;

void Editor::EditorLayer::Init(Core::EngineContext &ctx) {
    m_Context = ctx;

    m_Context.camera = &m_Camera;
    m_Context.sceneManager = &m_SceneManager;

    ctx.camera = &m_Camera;
    ctx.sceneManager = &m_SceneManager;

    m_SceneConfigEditor.SetContext(m_Context);
    m_ProjectConfigEditor.SetContext(m_Context);

    m_SceneManager.SetContext(m_Context);

    m_Input = m_Context.input;

    if (Core::Project::GetActive()) {
        LoadStartScene();
        if (!m_PendingSceneToLoad.empty()) {
            LoadScene(m_PendingSceneToLoad);
            m_PendingSceneToLoad.clear();
        }
    } else {
        std::cout << "[EditorLayer] No active project" << std::endl;
    }

    m_CurrentState = std::make_unique<EditState>();
    m_CurrentState->SetEditorLayer(this);
    m_CurrentState->OnEnter();
}


void Editor::EditorLayer::Update(const float dt) {
    if (!m_PendingSceneToLoad.empty()) {
        LoadScene(m_PendingSceneToLoad);
        m_PendingSceneToLoad.clear();
    }

    if (!m_Scene || !m_Registry)
        return;

    m_CurrentState->OnUpdate(dt);

    HandleInput(dt);
}

void Editor::EditorLayer::Render() {
    ImGuizmo::BeginFrame();
    DrawInterface();

    if (!Core::Project::GetActive())
        return;

    if (m_Context.camera) {
        aspect = m_ViewportPanel.GetWidth() / m_ViewportPanel.GetHeight();
        m_Context.renderer->SetCamera(*m_Context.camera, aspect);
    }

    m_SceneManager.Render();
}

void Editor::EditorLayer::Shutdown() {
}

void Editor::EditorLayer::HandleInput(const float dt) {
    // mode-independent hotkeys
    if (m_Input->IsKeyPressed("Esc")) {
        const bool hasChanges = (m_Scene && m_Scene->HasUnsavedChanges()) ||
                                (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges());
        if (hasChanges) {
            m_SaveChangesDialog.SetMessage("Do you want to save before quitting?");
            m_SaveChangesDialog.SetOnSave([this] {
                if (m_Scene &&m_Scene
                ->
                HasUnsavedChanges()
                )
                {
                    SaveScene();
                }
                if (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges()) {
                    m_ProjectConfigEditor.SaveConfig();
                }
                m_Context.window->Close();
            });
            m_SaveChangesDialog.SetOnDiscard([this] { m_Context.window->Close(); });
            m_SaveChangesDialog.Open();
        } else {
            m_Context.window->Close();
        }
        return;
    }

    if (m_Input->IsKeyPressed("F1")) {
        if (m_CurrentState->IsPlayMode())
            TransitionTo(std::make_unique<EditState>());
        return;
    }

    if (m_Input->IsKeyPressed("F5")) {
        if (m_CurrentState->IsPlayMode()) {
            TransitionTo(std::make_unique<EditState>());
        } else {
            if (m_CurrentScenePath.empty()) {
                std::cerr << "[Editor] Cannot enter Play Mode: scene has not been saved yet.\n";
                return;
            }
            if (m_Scene &&m_Scene
            ->
            HasUnsavedChanges()
            )
            {
                m_SaveChangesDialog.SetMessage(
                    "Do you want to save changes to '" + m_Scene->GetProperties().Name +
                    "' before entering Play Mode?\n\nUnsaved changes will be lost when stopping play.");
                m_SaveChangesDialog.SetOnSave([this] {
                    SaveScene();
                    TransitionTo(std::make_unique<PlayState>());
                });
                m_SaveChangesDialog.SetOnDiscard([this] { TransitionTo(std::make_unique<PlayState>()); });
                m_SaveChangesDialog.Open();
            }
            else
            {
                TransitionTo(std::make_unique<PlayState>());
            }
        }
        return;
    }

    m_CurrentState->OnHandleInput(dt);
}

// ReSharper disable once CppPassValueParameterByConstReference
void Editor::EditorLayer::LoadScene(std::string path) {
    std::cout << "[LoadScene] Loading scene: " << path << std::endl;

    ClearCurrentProject();

    // Initialize script pool BEFORE loading the scene so Scene::OnEnter()
    // can register EngineLib native functions on workers.
    // The script root path requires VFS to be mounted (project must be loaded).
    if (Core::Project::GetActive()) {
        m_Context.scriptPool->init(IO::VFS::GetAssetsDirectory() / "scripts");
        m_Context.scriptPool->set_stdout(m_InterpreterOutput);
    }

    try {
        m_SceneManager.LoadSceneByPath(path);
        m_Scene = m_SceneManager.GetCurrentScene();

        if (m_Scene) {
            m_Registry = &m_Scene->GetRegistry();
            m_CurrentScenePath = path;
            m_Scene->GetContext().camera = &m_Camera;
            m_SceneConfigEditor.ReloadFromScene();

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
    m_ShowSceneConfig = false;
    m_ShowProjectConfig = false;

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

void Editor::EditorLayer::SaveScene() const { static_cast<void>(m_SceneManager.SaveCurrentScene()); }

void Editor::EditorLayer::TransitionTo(std::unique_ptr<EditorState> newState) {
    if (m_CurrentState)
        m_CurrentState->OnExit();
    m_CurrentState = std::move(newState);
    m_CurrentState->SetEditorLayer(this);
    m_CurrentState->OnEnter();
}

void Editor::EditorLayer::DrawInterface() {
    if (!Core::Project::GetActive()) {
        DrawProjectHub();
    } else {
        DrawDockSpace();
        DrawToolbar();
        DrawEditorPanels();
        DrawUtilityWindows();
    }

    // property windows
    m_SceneConfigEditor.OnImGuiRender(m_ShowSceneConfig);
    m_ProjectConfigEditor.OnImGuiRender(m_ShowProjectConfig);

    // encapsulated dialogs
    m_NewProjectDialog.Update();
    if (Core::Project::GetActive()) {
        m_CreateSceneDialog.Update();
        m_SaveSceneAsDialog.Update();
    }
    m_SaveChangesDialog.Update();
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
        if (const auto dir = FileDialogs::PickFolder(m_Context)) {
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
        if (const auto dir = FileDialogs::PickFolder(m_Context)) {
            if (const std::filesystem::path projectFile = std::filesystem::path(*dir) / "project.json";
                std::filesystem::exists(projectFile)) {
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

    ImGui::DockBuilderFinish(dockspaceId);
    s_ShouldBuildDock = false;
}

void Editor::EditorLayer::DrawEditorPanels() {
    m_RegistryPanel.SetContext(m_Scene, m_Context);
    m_InspectorPanel.SetContext(m_Scene, m_Context);
    m_ViewportPanel.SetContext(m_Scene, m_Context);

    m_InspectorPanel.SetSelectedEntity(m_RegistryPanel.GetSelectedEntity());

    // viewport must be rendered first so ImGuizmo::SetDrawlist/SetRect are called
    m_ViewportPanel.OnImGuiRender();
    m_ViewportPanel.SetPlayModeIndicator(m_CurrentState->IsPlayMode());

    m_CurrentState->OnDrawPanels();
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
                if (const auto dir = FileDialogs::PickFolder(m_Context)) {
                    const std::filesystem::path pickedDir(*dir);

                    auto onProceed = [this, pickedDir] {
                        m_NewProjectDialog.SetDirectory(pickedDir);
                        m_NewProjectDialog.SetOnConfirm(
                            [this](const std::filesystem::path &pDir, const std::string &name) {
                                const auto newProject = Core::Project::NewProject(pDir, name);
                                if (newProject) {
                                    LoadProject(newProject->GetProjectPath().string());
                                }
                            });
                        m_NewProjectDialog.Open();
                    };

                    const bool hasChanges =
                            (m_Scene && m_Scene->HasUnsavedChanges()) ||
                            (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges());
                    if (hasChanges) {
                        m_SaveChangesDialog.SetMessage("Do you want to save before creating a new project?");
                        m_SaveChangesDialog.SetOnSave([this, onProceed] {
                            if (m_Scene &&m_Scene
                            ->
                            HasUnsavedChanges()
                            )
                            {
                                SaveScene();
                            }
                            if (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges()) {
                                m_ProjectConfigEditor.SaveConfig();
                            }
                            onProceed();
                        });
                        m_SaveChangesDialog.SetOnDiscard([this, onProceed] { onProceed(); });
                        m_SaveChangesDialog.Open();
                    } else {
                        onProceed();
                    }
                }
            }

            if (ImGui::MenuItem("Open Project")) {
                if (const auto dir = FileDialogs::PickFolder(m_Context)) {
                    const std::filesystem::path projectFile = std::filesystem::path(*dir) / "project.json";
                    if (!std::filesystem::exists(projectFile)) {
                        std::cerr << "[Editor] No project.json found in: " << *dir << "\n";
                    } else if ((m_Scene && m_Scene->HasUnsavedChanges()) ||
                               (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges())) {
                        m_SaveChangesDialog.SetMessage("Do you want to save before opening another project?");
                        m_SaveChangesDialog.SetOnSave([this, projectFile] {
                            if (m_Scene &&m_Scene
                            ->
                            HasUnsavedChanges()
                            )
                            {
                                SaveScene();
                            }
                            if (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges()) {
                                m_ProjectConfigEditor.SaveConfig();
                            }
                            LoadProject(projectFile.string());
                        });
                        m_SaveChangesDialog.SetOnDiscard([this, projectFile] { LoadProject(projectFile.string()); });
                        m_SaveChangesDialog.Open();
                    } else {
                        LoadProject(projectFile.string());
                    }
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Save Scene", "Ctrl+S", nullptr, m_CurrentState->CanSaveScene())) {
                if (m_CurrentState->CanSaveScene())
                    SaveScene();
            }

            if (ImGui::MenuItem("Save Scene As...", nullptr, nullptr, m_CurrentState->CanSaveSceneAs())) {
                std::string currentName = "scene";
                if (m_Scene && !m_Scene->GetProperties().Name.empty()) {
                    currentName = m_Scene->GetProperties().Name;
                }

                m_SaveSceneAsDialog.SetCurrentName(currentName);
                m_SaveSceneAsDialog.SetOnConfirm([this](const std::string &newName) {
                    std::string safeName = newName;
                    std::ranges::replace(safeName, ' ', '_');
                    if (const std::string scenePath = Core::PathUtils::Join(Core::SCENE_PATH, safeName, ".json");
                        IO::SceneIO::Serialize(scenePath, *m_Scene)) {
                        m_Scene->GetProperties().ScenePath = scenePath;
                        m_Scene->GetProperties().Name = newName;
                    }
                });
                m_SaveSceneAsDialog.Open();
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Project Settings")) {
                m_ProjectConfigEditor.Reload();
                m_ShowProjectConfig = true;
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Close Project")) {
                const bool hasChanges = (m_Scene && m_Scene->HasUnsavedChanges()) ||
                                        (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges());
                if (hasChanges) {
                    m_SaveChangesDialog.SetMessage("Do you want to save before closing the project?");
                    m_SaveChangesDialog.SetOnSave([this] {
                        if (m_Scene &&m_Scene
                        ->
                        HasUnsavedChanges()
                        )
                        {
                            SaveScene();
                        }
                        if (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges()) {
                            m_ProjectConfigEditor.SaveConfig();
                        }
                        ClearCurrentProject();
                        Core::Project::SetActive(nullptr);
                        IO::VFS::UnmountProject();
                        s_ShouldBuildDock = true;
                    });
                    m_SaveChangesDialog.SetOnDiscard([this] {
                        ClearCurrentProject();
                        Core::Project::SetActive(nullptr);
                        IO::VFS::UnmountProject();
                        s_ShouldBuildDock = true;
                    });
                    m_SaveChangesDialog.Open();
                } else {
                    ClearCurrentProject();
                    Core::Project::SetActive(nullptr);
                    IO::VFS::UnmountProject();
                    s_ShouldBuildDock = true;
                }
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Export Project")) {
                if (auto path = FileDialogs::PickFolder(m_Context, nullptr, "Pick export directory")) {
                    IO::Package::Tools::ExportGame(path.value());
                }
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) {
                const bool hasChanges = (m_Scene && m_Scene->HasUnsavedChanges()) ||
                                        (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges());
                if (hasChanges) {
                    m_SaveChangesDialog.SetMessage("Do you want to save before quitting?");
                    m_SaveChangesDialog.SetOnSave([this] {
                        if (m_Scene &&m_Scene
                        ->
                        HasUnsavedChanges()
                        )
                        {
                            SaveScene();
                        }
                        if (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges()) {
                            m_ProjectConfigEditor.SaveConfig();
                        }
                        m_Context.window->Close();
                    });
                    m_SaveChangesDialog.SetOnDiscard([this] { m_Context.window->Close(); });
                    m_SaveChangesDialog.Open();
                } else {
                    m_Context.window->Close();
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scene")) {
            if (ImGui::MenuItem("Edit Scene Properties")) {
                m_SceneConfigEditor.ReloadFromScene();
                m_ShowSceneConfig = true;
            }
            ImGui::Separator();


            if (ImGui::MenuItem("Create Scene")) {
                m_CreateSceneDialog.Reset();
                m_CreateSceneDialog.SetOnConfirm([this](const std::string &sceneName) {
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
                            if (m_Scene &&m_Scene
                            ->
                            HasUnsavedChanges()
                            )
                            {
                                m_SaveChangesDialog.SetMessage("Do you want to save changes to '" +
                                                               m_Scene->GetProperties().Name +
                                                               "' before switching scenes?");
                                m_SaveChangesDialog.SetOnSave([this, scenePath] {
                                    SaveScene();
                                    LoadScene(scenePath);
                                });
                                m_SaveChangesDialog.SetOnDiscard([this, scenePath] { LoadScene(scenePath); });
                                m_SaveChangesDialog.Open();
                            }
                            else
                            {
                                m_PendingSceneToLoad = scenePath;
                            }
                        }
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        const float buttonWidth = 70.0f;
        const float centerPos = ImGui::GetWindowSize().x * 0.5f - buttonWidth * 0.5f;

        ImGui::SetCursorPosX(centerPos);

        if (ImGui::Button(m_CurrentState->PlayStopLabel(), ImVec2(buttonWidth, 0))) {
            if (m_CurrentState->IsPlayMode()) {
                TransitionTo(std::make_unique<EditState>());
            } else {
                if (m_CurrentScenePath.empty()) {
                    std::cerr << "[Editor] Cannot enter Play Mode: scene has not been saved yet.\n";
                } else if (m_Scene &&m_Scene
                ->
                HasUnsavedChanges()
                )
                {
                    m_SaveChangesDialog.SetMessage(
                        "Do you want to save changes to '" + m_Scene->GetProperties().Name +
                        "' before entering Play Mode?\n\nUnsaved changes will be discarded.");
                    m_SaveChangesDialog.SetOnSave([this] {
                        SaveScene();
                        TransitionTo(std::make_unique<PlayState>());
                    });
                    m_SaveChangesDialog.SetOnDiscard([this] { TransitionTo(std::make_unique<PlayState>()); });
                    m_SaveChangesDialog.Open();
                }
                else
                {
                    TransitionTo(std::make_unique<PlayState>());
                }
            }
        }
        ImGui::EndMainMenuBar();
    }
}
