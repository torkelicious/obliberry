#include "EditorLayer.h"
#include "Core/Project.h"
#include "Core/ProjectConfig.h"
#include "Core/LoggerService.h"
#include "ECS/Entity.h"
#include "ECS/Systems/LightingSystem.h"
#include "Scenes/Scene.h"
#include "imgui.h"
#include "imgui_internal.h"

#include <ObSL/ScriptRuntime.h>
#include <filesystem>
#include <memory>
#include <string>
#include <algorithm>

#include "Core/InputManager.h"
#include "Core/Window.h"
#include "FileDialogs.h"
#include "Commands/EditorCommands.h"
#include "IO/SceneSerialization.h"
#include "IO/MapSerialization.h"
#include "IO/VFS.h"
#include "IO/Package/Tools/ObpakTools.h"

#include "States/EditState.h"
#include "States/PlayState.h"
#include "States/MapEditState.h"
#include "States/HubState.h"

/* TODO:
 * map editor - wip
 * project browser (asset view ig) - wip
 * whatever else an editor needs?
 * TODO/FIX: Reload map if changes unsaved, otherwise innacurate scene view in relation to actual file
 */

bool Editor::EditorLayer::s_ShouldBuildDock = true;


#pragma push_macro("LOG_WHO")
#define LOG_WHO "EditorLayer"

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
        m_CurrentState = std::make_unique<EditState>();
    } else {
        LOG_INFO(LOG_WHO, "No active project");
        m_CurrentState = std::make_unique<HubState>();
    }

    m_CurrentState->SetEditorLayer(this);
    m_CurrentState->OnEnter();

    m_ProjectConfigEditor.SetUndoMgr(&m_UndoManager);
    m_SceneConfigEditor.SetUndoMgr(&m_UndoManager);
}


void Editor::EditorLayer::Update(const float dt) {
    if (!m_PendingSceneToLoad.empty()) {
        LoadScene(m_PendingSceneToLoad);
        m_PendingSceneToLoad.clear();
    }

    ExecutePendingStateTransfer();

    if (!m_Scene || !m_Registry)
        return;

    m_CurrentState->OnUpdate(dt);
    if (m_CurrentState->GetWindowTitleDirty()) {
        m_Context.window->SetWindowTitle(m_CurrentState->GetWindowTitle());
        m_CurrentState->SetWindowTitleDirty(false);
    }

    HandleInput(dt);

    m_Camera.UpdateSmooth(dt);
}

void Editor::EditorLayer::Render() {
    if (m_CurrentState)
        m_CurrentState->OnRender();

    // common UI rendered regardless of state
    m_SceneConfigEditor.OnImGuiRender(m_ShowSceneConfig);
    m_ProjectConfigEditor.OnImGuiRender(m_ShowProjectConfig);

    m_NewProjectDialog.Update();
    if (Core::Project::GetActive()) {
        m_CreateSceneDialog.Update();
        m_SaveSceneAsDialog.Update();
    }
    m_SaveChangesDialog.Update();
    m_SaveMapDialog.Update();
}

void Editor::EditorLayer::Shutdown() {}

void Editor::EditorLayer::HandleInput(const float dt) {
    // NOTE:
    // ImGui starts a "window move" when left clicking the Scene View window,
    // this sets g.ActiveId to the window's MoveId.
    // Even though the move is cancelled immediately
    // (the click was on the framebuffer, not the tab),
    // but, ActiveId remains set for the next frame.
    // This causes WantCaptureKeyboard to become true and blocks HandleInput()
    // before it can reach OnHandleInput().
    //
    // Use WantTextInput instead since it
    // only triggers for text input and not any active ImGui ID.
    // I spent like 4 hours trying to figure out why my clicks werent working..
    // -.-
    if (ImGui::GetIO().WantTextInput)
        return;

    // mode-independent hotkeys
    if (m_Input->IsKeyPressed("Esc")) {
        if (const bool hasChanges = (m_Scene && m_Scene->HasUnsavedChanges()) || (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges())) {
            m_SaveChangesDialog.SetMessage("Do you want to save before quitting?");
            m_SaveChangesDialog.SetOnSave([this] {
                if (m_Scene && m_Scene->HasUnsavedChanges()) {
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

    //
    // 2key Modifier Shortcuts
    //
    if (m_Input->IsKeyComboPressed({"LeftCtrl", "S"})) {
        if (m_CurrentState->CanSaveScene()) {
            SaveScene();
        }
    }
    if (m_Input->IsKeyComboPressed({"LeftCtrl", "Z"})) {
        if (m_UndoManager.CanUndo()) {
            m_UndoManager.Undo(m_Context);
        }
    }
    if (m_Input->IsKeyComboPressed({"LeftCtrl", "Y"})) {
        if (m_UndoManager.CanRedo()) {
            m_UndoManager.Redo(m_Context);
        }
    }

    //
    // Function keys
    //
    if (m_Input->IsKeyPressed("F1")) {
        if (m_CurrentState->IsPlayMode())
            TransitionTo(m_PreviousState ? std::move(m_PreviousState) : std::make_unique<EditState>());
        return;
    }

    if (m_Input->IsKeyPressed("F5")) {
        if (m_CurrentState->IsPlayMode()) {
            TransitionTo(m_PreviousState ? std::move(m_PreviousState) : std::make_unique<EditState>());
        } else {
            if (m_CurrentScenePath.empty()) {
                LOG_ERROR("Editor", "Cannot enter Play Mode: scene has not been saved yet");
                return;
            }
            if (m_Scene && m_Scene->HasUnsavedChanges()) {
                m_SaveChangesDialog.SetMessage("Do you want to save changes to '" + m_Scene->GetProperties().Name + "' before entering Play Mode?\n\nUnsaved changes will be lost when stopping play.");
                m_SaveChangesDialog.SetOnSave([this] {
                    SaveScene();
                    TransitionTo(std::make_unique<PlayState>());
                });
                m_SaveChangesDialog.SetOnDiscard([this] { TransitionTo(std::make_unique<PlayState>()); });
                m_SaveChangesDialog.Open();
            } else {
                TransitionTo(std::make_unique<PlayState>());
            }
        }
        return;
    }

    m_CurrentState->OnHandleInput(dt);
}

// ReSharper disable once CppPassValueParameterByConstReference
void Editor::EditorLayer::LoadScene(std::string path) {
    LOG_INFO("LoadScene", "Loading scene: " + path);

    ClearCurrentProject();

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

            LOG_INFO("LoadScene", "Successfully loaded scene with " + std::to_string(m_Scene->GetRegistry().GetLivingEntities().size()) + " entities");
            Commands::RefreshWindowTitle(m_Context);

        } else {
            LOG_ERROR("LoadScene", "Failed to load scene: " + path);
            m_Registry = nullptr;
            m_CurrentScenePath.clear();
        }
    } catch (const std::exception &e) {
        LOG_ERROR("LoadScene", "Exception while loading scene: " + std::string(e.what()));
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

    m_UndoManager.Clear();
}

void Editor::EditorLayer::LoadProject(const std::string &projectFilePath) {
    ClearCurrentProject();

    IO::VFS::UnmountProject();

    Core::Project::Load(projectFilePath);

    // sync loaded config into EngineContext
    *m_Context.projectConfig = Core::Project::GetActive()->GetConfig();

    LoadStartScene();

    TransitionTo(std::make_unique<EditState>());
}

void Editor::EditorLayer::LoadStartScene() {
    if (!Core::Project::GetActive()) {
        LOG_ERROR("LoadStartScene", "No active project!");
        return;
    }

    const std::string startScene = Core::Project::GetActive()->GetConfig().startScenePath;
    if (startScene.empty()) {
        LOG_WARN("LoadStartScene", "startScenePath is empty in project configuration!");
        return;
    }

    LOG_INFO("LoadStartScene", "Loading start scene: " + startScene);
    m_PendingSceneToLoad = startScene;
}

void Editor::EditorLayer::SaveScene() const { static_cast<void>(m_SceneManager.SaveCurrentScene()); }

void Editor::EditorLayer::TransitionTo(std::unique_ptr<EditorState> newState) { m_PendingState = std::move(newState); }

void Editor::EditorLayer::ExecutePendingStateTransfer() {
    if (!m_PendingState)
        return;

    if (m_CurrentState)
        m_CurrentState->OnExit();

    // save the current state  so it can be returned to on exiting play mode
    if (m_PendingState->IsPlayMode())
        m_PreviousState = std::move(m_CurrentState);

    m_CurrentState = std::move(m_PendingState);
    m_CurrentState->SetEditorLayer(this);
    m_CurrentState->OnEnter();
    m_PendingState = nullptr;

    m_UndoManager.Clear();
}

void Editor::EditorLayer::PromptSaveDirtyMap(const std::function<void()> &onProceed) {
    if (const auto *mapState = dynamic_cast<MapEditState *>(m_CurrentState.get()); !mapState || !m_Registry) {
        onProceed();
        return;
    }

    bool isDirty = false;
    m_Registry->ForEach<ECS::Components::MapComponent>([&](ECS::Entity, const ECS::Components::MapComponent *mapComp) {
        if (mapComp->mapDirty)
            isDirty = true;
    });

    if (!isDirty) {
        onProceed();
        return;
    }

    m_SaveMapDialog.SetMessage("Map has unsaved changes. Save before leaving?");
    m_SaveMapDialog.SetOnSave([this, onProceed] {
        m_Registry->ForEach<ECS::Components::MapComponent>([&](ECS::Entity, ECS::Components::MapComponent *mapComp) {
            if (mapComp->mapDirty && !mapComp->mapFilePath.empty()) {
                if (IO::MapIO::Serialize(mapComp->mapFilePath, mapComp->grid)) {
                    mapComp->mapDirty = false;
                }
            }
        });
        onProceed();
    });
    m_SaveMapDialog.SetOnDiscard([this, onProceed] {
        m_Registry->ForEach<ECS::Components::MapComponent>([&](ECS::Entity, ECS::Components::MapComponent *mapComp) { mapComp->mapDirty = false; });
        onProceed();
    });
    m_SaveMapDialog.Open();
}

void Editor::EditorLayer::DrawEditorUI() {
    DrawDockSpace();
    DrawToolbar();
    DrawEditorPanels();
    DrawUtilityWindows();

    if (m_Context.camera) {
        const float aspect = m_ViewportPanel.GetWidth() / m_ViewportPanel.GetHeight();
        m_Context.renderer->SetCamera(*m_Context.camera, aspect);
    }
}

void Editor::EditorLayer::DrawEditorLayout() {
    DrawEditorUI();
    m_SceneManager.Render();
}

void Editor::EditorLayer::DrawProjectHub() {
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
    ImGui::Begin("Obliberry hub", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

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
            if (const std::filesystem::path projectFile = std::filesystem::path(*dir) / "project.json"; std::filesystem::exists(projectFile)) {
                LoadProject(projectFile.string());
            } else {
                LOG_ERROR("Hub", "No project.json found in: " + *dir);
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
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_PassthruCentralNode | static_cast<int>(ImGuiDockNodeFlags_DockSpace));
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

    ImGuiID dock_id_center = dockspaceId;

    const ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Right, 0.25f, nullptr, &dock_id_center);
    const ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Left, 0.2f, nullptr, &dock_id_center);
    const ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Down, 0.3f, nullptr, &dock_id_center);

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
    m_InspectorPanel.SetContext(m_Scene, m_Context, &m_UndoManager);
    m_ProjectBrowserPanel.SetContext(m_Scene, m_Context);
    m_ViewportPanel.SetContext(m_Scene, m_Context);

    m_InspectorPanel.SetSelectedEntity(m_RegistryPanel.GetSelectedEntity());

    // viewport must be rendered first so ImGuizmo::SetDrawlist/SetRect are called
    m_ViewportPanel.OnImGuiRender();
    if (m_CurrentState) {
        m_ViewportPanel.SetPlayModeIndicator(m_CurrentState->IsPlayMode());
        m_CurrentState->OnDrawPanels();
    }
}


void Editor::EditorLayer::DrawUtilityWindows() {
    FlushInterpreterOutput();

    ImGui::Begin("Console");

    if (ImGui::Button("Clear")) {
        m_ConsoleLogs.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Copy")) {
        std::string allText;
        for (const auto &line : m_ConsoleLogs) {
            allText += line;
            allText += '\n';
        }
        ImGui::SetClipboardText(allText.c_str());
    }

    ImGui::Separator();

    ImGui::BeginChild("##console_scroll", ImVec2(-1, -1), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto &line : m_ConsoleLogs) {
        ImGui::TextUnformatted(line.c_str());
    }
    if (m_ConsoleLogs.size() != m_PreviousLogCount) {
        ImGui::SetScrollHereY(1.0f);
        m_PreviousLogCount = m_ConsoleLogs.size();
    }
    ImGui::EndChild();

    ImGui::End();

    if (m_CurrentState && m_CurrentState->ShouldDrawProjectBrowser()) {
        m_ProjectBrowserPanel.OnImGuiRender();
    }

    m_CurrentState->OnDrawUtilityWindows();
}

void Editor::EditorLayer::DrawToolbar() {
    if (ImGui::BeginMainMenuBar()) {

        // undomgr btns

        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, m_UndoManager.CanUndo())) {
            m_UndoManager.Undo(m_Context);
        }

        if (ImGui::MenuItem("Redo", "Ctrl+Y", false, m_UndoManager.CanRedo())) {
            m_UndoManager.Redo(m_Context);
        }

        ImGui::Separator();

        // file

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project")) {
                if (const auto dir = FileDialogs::PickFolder(m_Context)) {
                    const std::filesystem::path pickedDir(*dir);

                    auto onProceed = [this, pickedDir] {
                        m_NewProjectDialog.SetDirectory(pickedDir);
                        m_NewProjectDialog.SetOnConfirm([this](const std::filesystem::path &pDir, const std::string &name) {
                            const auto newProject = Core::Project::NewProject(pDir, name);
                            if (newProject) {
                                LoadProject(newProject->GetProjectPath().string());
                            }
                        });
                        m_NewProjectDialog.Open();
                    };

                    if (const bool hasChanges = (m_Scene && m_Scene->HasUnsavedChanges()) || (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges())) {
                        m_SaveChangesDialog.SetMessage("Do you want to save before creating a new project?");
                        m_SaveChangesDialog.SetOnSave([this, onProceed] {
                            if (m_Scene && m_Scene->HasUnsavedChanges()) {
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
                    if (const std::filesystem::path projectFile = std::filesystem::path(*dir) / "project.json"; !std::filesystem::exists(projectFile)) {
                        LOG_ERROR("Editor", "No project.json found in: " + *dir);
                    } else if ((m_Scene && m_Scene->HasUnsavedChanges()) || (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges())) {
                        m_SaveChangesDialog.SetMessage("Do you want to save before opening another project?");
                        m_SaveChangesDialog.SetOnSave([this, projectFile] {
                            if (m_Scene && m_Scene->HasUnsavedChanges()) {
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
                    if (const std::string scenePath = Core::PathUtils::Join(Core::SCENE_PATH, safeName, ".json"); IO::SceneIO::Serialize(scenePath, *m_Scene)) {
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
                if (const bool hasChanges = (m_Scene && m_Scene->HasUnsavedChanges()) || (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges())) {
                    m_SaveChangesDialog.SetMessage("Do you want to save before closing the project?");
                    m_SaveChangesDialog.SetOnSave([this] {
                        if (m_Scene && m_Scene->HasUnsavedChanges()) {
                            SaveScene();
                        }
                        if (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges()) {
                            m_ProjectConfigEditor.SaveConfig();
                        }
                        ClearCurrentProject();
                        Core::Project::SetActive(nullptr);
                        IO::VFS::UnmountProject();
                        s_ShouldBuildDock = true;
                        TransitionTo(std::make_unique<HubState>());
                    });
                    m_SaveChangesDialog.SetOnDiscard([this] {
                        ClearCurrentProject();
                        Core::Project::SetActive(nullptr);
                        IO::VFS::UnmountProject();
                        s_ShouldBuildDock = true;
                        TransitionTo(std::make_unique<HubState>());
                    });
                    m_SaveChangesDialog.Open();
                } else {
                    PromptSaveDirtyMap([this] {
                        ClearCurrentProject();
                        Core::Project::SetActive(nullptr);
                        IO::VFS::UnmountProject();
                        s_ShouldBuildDock = true;
                        TransitionTo(std::make_unique<HubState>());
                    });
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
                if (const bool hasChanges = (m_Scene && m_Scene->HasUnsavedChanges()) || (Core::Project::GetActive() && Core::Project::GetActive()->HasUnsavedChanges())) {
                    m_SaveChangesDialog.SetMessage("Do you want to save before quitting?");
                    m_SaveChangesDialog.SetOnSave([this] {
                        if (m_Scene && m_Scene->HasUnsavedChanges()) {
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
                m_CreateSceneDialog.SetOnConfirm([this](const std::string &sceneName) { static_cast<void>(m_SceneManager.CreateNewScene(sceneName)); });
                m_CreateSceneDialog.Open();
            }
            ImGui::Separator();

            if (ImGui::BeginMenu("Switch To")) {
                if (const auto scenes = Scenes::SceneManager::GetAvailableScenes(); scenes.empty()) {
                    ImGui::MenuItem("(no scenes)", nullptr, false, false);
                } else {
                    for (const auto &scenePath : scenes) {
                        if (const bool isCurrent = m_Scene && m_Scene->GetScenePath() == scenePath; ImGui::MenuItem(scenePath.c_str(), nullptr, false, !isCurrent)) {
                            PromptSaveDirtyMap([this, scenePath] {
                                if (m_Scene && m_Scene->HasUnsavedChanges()) {
                                    m_SaveChangesDialog.SetMessage("Do you want to save changes to '" + m_Scene->GetProperties().Name + "' before switching scenes?");
                                    m_SaveChangesDialog.SetOnSave([this, scenePath] {
                                        SaveScene();
                                        LoadScene(scenePath);
                                    });
                                    m_SaveChangesDialog.SetOnDiscard([this, scenePath] { LoadScene(scenePath); });
                                    m_SaveChangesDialog.Open();
                                } else {
                                    LoadScene(scenePath);
                                }
                            });
                            m_PendingSceneToLoad = scenePath;
                        }
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        m_CurrentState->OnDrawModeToolbar();

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // mode selector
        {
            const bool inPlayMode = m_CurrentState->IsPlayMode();
            ImGui::BeginDisabled(inPlayMode);

            const char *modeItems[] = {"Edit", "Map Edit"};
            int currentMode = 0;
            if (dynamic_cast<MapEditState *>(m_CurrentState.get()))
                currentMode = 1;
            ImGui::SetNextItemWidth(110.0f);
            if (ImGui::Combo("##Mode", &currentMode, modeItems, IM_ARRAYSIZE(modeItems))) {
                switch (currentMode) {
                    default:
                    case 0:
                        PromptSaveDirtyMap([this] { TransitionTo(std::make_unique<EditState>()); });
                        break;
                    case 1:
                        PromptSaveDirtyMap([this] { TransitionTo(std::make_unique<MapEditState>()); });
                        break;
                }
            }

            ImGui::EndDisabled();

            if (inPlayMode && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("Stop Play mode first to change editor mode");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        const float buttonWidth = 70.0f;
        const float centerPos = ImGui::GetWindowSize().x * 0.5f - buttonWidth * 0.5f;

        ImGui::SetCursorPosX(centerPos);

        if (ImGui::Button(m_CurrentState->PlayStopLabel(), ImVec2(buttonWidth, 0))) {
            if (m_CurrentState->IsPlayMode()) {
                TransitionTo(m_PreviousState ? std::move(m_PreviousState) : std::make_unique<EditState>());
            } else {
                PromptSaveDirtyMap([this] {
                    if (m_Scene && m_Scene->HasUnsavedChanges()) {
                        m_SaveChangesDialog.SetMessage("Scene has unsaved changes. Save before playing?");
                        m_SaveChangesDialog.SetOnSave([this] {
                            SaveScene();
                            TransitionTo(std::make_unique<PlayState>());
                        });
                        m_SaveChangesDialog.SetOnDiscard([this] { TransitionTo(std::make_unique<PlayState>()); });
                        m_SaveChangesDialog.Open();
                    } else {
                        TransitionTo(std::make_unique<PlayState>());
                    }
                });
            }
                    }
                }
                ImGui::EndMainMenuBar();
            }
            #pragma pop_macro("LOG_WHO")
