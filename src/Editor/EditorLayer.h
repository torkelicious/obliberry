#pragma once
#include <cstdint>
#include <memory>

#include "Core/ApplicationLayer.h"
#include "UI/Panels/Editor/InspectorPanel.h"
#include "UI/Panels/Editor/ProjectBrowserPanel.h"
#include "UI/Panels/Editor/RegistryPanel.h"
#include "UI/Panels/ViewportPanel.h"

#include "EditorCamera.h"
#include "EditorState.h"
#include "Commands/UndoManager.h"
#include "Map/HexCoords.h"
#include "Scenes/Scene.h"
#include "Scenes/SceneManager.h"
#include "UI/Modals/StdDialogs.h"
#include "UI/ConfigWindows/ProjectConfigEditor.h"
#include "UI/ConfigWindows/SceneConfigEditor.h"

namespace Editor {
    class EditState;
    class PlayState;
    class MapEditState;

    class EditorLayer : public Core::ApplicationLayer {
        friend class EditorState;
        friend class EditState;
        friend class PlayState;
        friend class MapEditState;
        friend class HubState;

    public:
        void Init(Core::EngineContext &ctx) override;

        void Update(float dt) override;

        void Render() override;

        void Shutdown() override;

    private:
        void DrawDockSpace();

        void DrawEditorPanels();

        void DrawUtilityWindows();

        void DrawEditorUI();

        void DrawEditorLayout();

        void DrawToolbar();

        void DrawProjectHub();

        void LoadProject(const std::string &projectFilePath);

        void ClearCurrentProject();

        void LoadStartScene();

        void HandleInput(float dt);

        void LoadScene(std::string path);

        void SaveScene() const;

        void TransitionTo(std::unique_ptr<EditorState> newState);
        void ExecutePendingStateTransfer();

        Core::EngineContext m_Context;
        Scenes::Scene *m_Scene = nullptr;
        std::string m_CurrentScenePath;
        EditorCamera m_Camera;
        Core::InputManager *m_Input = nullptr;
        ECS::Registry *m_Registry = nullptr;
        Scenes::SceneManager m_SceneManager;

        std::unique_ptr<EditorState> m_CurrentState;
        std::unique_ptr<EditorState> m_PendingState;
        std::unique_ptr<EditorState> m_PreviousState;

        Map::HexCoords m_SelectedTile;

        std::string m_PendingSceneToLoad;
        // UI
        static bool s_ShouldBuildDock;
        UI::RegistryPanel m_RegistryPanel;
        UI::InspectorPanel m_InspectorPanel;
        UI::ProjectBrowserPanel m_ProjectBrowserPanel;
        UI::ViewportPanel m_ViewportPanel;
        UI::NewProjectDialog m_NewProjectDialog;
        UI::CreateSceneDialog m_CreateSceneDialog;
        UI::SaveChangesDialog m_SaveChangesDialog;
        UI::SaveSceneAsDialog m_SaveSceneAsDialog;
        UI::SceneConfigEditor m_SceneConfigEditor;
        UI::ProjectConfigEditor m_ProjectConfigEditor;
        bool m_ShowSceneConfig = false;
        bool m_ShowProjectConfig = false;

        // commands
        UndoManager m_UndoManager;

        // Logging
        std::vector<std::string> m_ConsoleLogs;
        std::stringstream m_InterpreterOutput;

        void FlushInterpreterOutput() {
            std::string line;
            while (std::getline(m_InterpreterOutput, line)) {
                m_ConsoleLogs.push_back(line);
            }
            m_InterpreterOutput.clear();
        }
    };
} // namespace Editor
