#pragma once
#include <memory>
#include <sstream>
#include "Core/ApplicationLayer.h"
#include "Applications/Editor/UI/Panels/Editor/InspectorPanel.h"
#include "UI/Panels/ProjectBrowserPanel.h"
#include "Applications/Editor/UI/Panels/Editor/RegistryPanel.h"
#include "Applications/Editor/UI/Panels/ViewportPanel.h"
#include "Applications/Editor/EditorCamera.h"
#include "Applications/Editor/States/EditorStateBase.h"
#include "Applications/Editor/Commands/UndoManager.h"
#include "Map/HexCoords.h"
#include "Scenes/Scene.h"
#include "Scenes/SceneManager.h"
#include "Applications/Editor/UI/Modals/StdDialogs.h"
#include "Applications/Editor/UI/ConfigWindows/ProjectConfigEditor.h"
#include "Applications/Editor/UI/ConfigWindows/SceneConfigEditor.h"
#include "UI/ConfigWindows/GraphicsConfigEditor.h"
#include "UI/Panels/Editor/UIPanel.h"
#include "Platform/Input/InputManager.h"

namespace Editor {
    namespace States {
        class EditState;
        class PlayState;
        class MapEditState;
        class HubState;
    } // namespace States

    class EditorLayer : public Core::ApplicationLayer {
        friend class States::EditorStateBase;
        friend class States::EditState;
        friend class States::PlayState;
        friend class States::MapEditState;
        friend class States::HubState;

    public:
        void Init(Core::EngineContext &ctx) override;

        void Update(float dt) override;

        void Render() override;

        void Shutdown() override;

        inline static bool s_ShouldBuildDock = true;

        inline static bool s_RenderParticlesInEditor = false;

    private:
        static void DrawDockSpace();

        void DrawEditorPanels();

        void DrawUtilityWindows();

        void DrawEditorUI();

        void DrawEditorLayout();

        void DrawToolbar();

        void LoadProject(const std::string &projectFilePath);

        void ClearCurrentProject();

        void LoadStartScene();

        void HandleInput(float dt);

        void LoadScene(std::string path);

        void SaveScene() const;

        void TransitionTo(std::unique_ptr<States::EditorStateBase> newState);
        void ExecutePendingStateTransfer();
        void PromptSaveDirtyMap(const std::function<void()> &onProceed);

        Core::EngineContext m_Context;
        Scenes::Scene *m_Scene = nullptr;
        std::string m_CurrentScenePath;
        EditorCamera m_Camera;
        Platform::Input::InputManager *m_Input = nullptr;
        ECS::Registry *m_Registry = nullptr;
        Scenes::SceneManager m_SceneManager;

        std::unique_ptr<States::EditorStateBase> m_CurrentState;
        std::unique_ptr<States::EditorStateBase> m_PendingState;
        std::unique_ptr<States::EditorStateBase> m_PreviousState;

        Map::HexCoords m_SelectedTile;

        std::string m_PendingSceneToLoad;
        // UI
        UI::RegistryPanel m_RegistryPanel;
        UI::InspectorPanel m_InspectorPanel;
        UI::ProjectBrowserPanel m_ProjectBrowserPanel;
        UI::ViewportPanel m_ViewportPanel;
        UI::UIPanel m_UIPanel;
        UI::NewProjectDialog m_NewProjectDialog;
        UI::CreateSceneDialog m_CreateSceneDialog;
        UI::SaveChangesDialog m_SaveChangesDialog;
        UI::SaveChangesDialog m_SaveMapDialog;
        UI::SaveSceneAsDialog m_SaveSceneAsDialog;
        UI::SceneConfigEditor m_SceneConfigEditor;
        UI::ProjectConfigEditor m_ProjectConfigEditor;
        UI::GraphicsConfigEditor m_GraphicsConfigEditor;
        bool m_ShowSceneConfig = false;
        bool m_ShowProjectConfig = false;
        bool m_ShowGraphicsConfig = false;

        // commands
        Commands::UndoManager m_UndoManager;

        // Logging
        std::vector<std::string> m_ConsoleLogs;
        size_t m_PreviousLogCount = 0;
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
