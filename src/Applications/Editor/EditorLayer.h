#pragma once
#include "Applications/Editor/States/Hub/ProjectHistory.h"
#include "Applications/Editor/UI/Panels/UiStream.h"
#include "Core/Utils/OsUtils.h"
#include "EditorContext.h"
#include <memory>
#include <string>
#include "Core/ApplicationLayer.h"
#include "Logger/LogSink.h"
#include "Platform/Input/InputManager.h"
#include "Applications/Editor/UI/Panels/Editor/InspectorPanel.h"
#include "UI/Panels/ProjectBrowserPanel.h"
#include "Applications/Editor/UI/Panels/Editor/RegistryPanel.h"
#include "Applications/Editor/UI/Panels/ViewportPanel.h"
#include "Applications/Editor/EditorCamera.h"
#include "Applications/Editor/States/EditorStateBase.h"
#include "Applications/Editor/Commands/UndoManager.h"
#include "Applications/Editor/Clipboard.h"
#include "Map/HexCoords.h"
#include "Scenes/Scene.h"
#include "Scenes/SceneManager.h"
#include "Applications/Editor/UI/Modals/StdDialogs.h"
#include "Applications/Editor/UI/ConfigWindows/ProjectConfigEditor.h"
#include "Applications/Editor/UI/ConfigWindows/SceneConfigEditor.h"
#include "UI/ConfigWindows/GraphicsConfigEditor.h"
#include "UI/ConfigWindows/PostProcEditor.h"
#include "UI/Panels/Editor/UIPanel.h"
#include "UI/ConfigWindows/ThemeConfigEditor.h"
#include "UI/Panels/ConsoleLogPanel.h"

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
        EditorLayer() = default;
        void Init(Core::EngineContext &ctx) override;

        void SetupFontSync(std::atomic<bool> *fontsDirty) override;

        void PreImGuiFrame() override;

        void SyncFonts(Core::EngineContext &ctx, std::mutex &imguiTextureMutex) override;

        void Update(float dt) override;

        void Render() override;

        void Shutdown() override;

        [[nodiscard]] bool UsesEditorViewport() const override { return true; }

        EditorContext *GetEditorContext() { return &m_EditorContext; }

        inline static bool s_ShouldBuildDock = true;

        inline static bool s_RenderParticlesInEditor = false;

        void ReloadCurrentMap() const;

    private:
        void DrawDockSpace() const;

        void DrawEditorPanels();

        void DrawUtilityWindows();

        void DrawEditorUI();

        void DrawEditorLayout();

        void DrawToolbar();

        void LoadProject(const std::string &projectFilePath);

        void ClearCurrentProject();

        void LoadStartScene();

        void HandleInput(float dt);

        void HandleClipboardInput();

        void LoadScene(std::string path);

        void SaveScene() const;

        void TransitionTo(std::unique_ptr<States::EditorStateBase> newState);
        void ExecutePendingStateTransfer();
        void PromptSaveDirtyMap(const std::function<void()> &onProceed);

        void ShowProjectLoadError(const std::string &ProjectPath, const std::string &why);
        void DrawProjectLoadErrorPopup();
        std::string m_ProjectLoadError;
        bool m_ProjectLoadErrorPending;


        Core::EngineContext *m_Context = nullptr;
        EditorContext m_EditorContext;
        Scenes::Scene *m_Scene = nullptr;
        std::string m_CurrentScenePath;
        EditorCamera m_Camera;
        ::Platform::Input::InputManager *m_Input = nullptr;
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
        UI::ThemeConfigEditor m_ThemeConfigEditor;
        UI::PostProcEditor m_PostProcConfigEditor;
        UI::MessageDialog m_ExportResultDialog{"Export Project"};
        bool m_ShowPostProcConfig = false;
        bool m_ShowSceneConfig = false;
        bool m_ShowProjectConfig = false;
        bool m_ShowGraphicsConfig = false;
        bool m_ShowThemeConfig = false;

        // commands
        Commands::UndoManager m_UndoManager;

        Clipboard m_Clipboard;

        // Project History
        ProjectHistory m_ProjectHistory;

        UI::ConsoleLogPanel m_InterpreterLogsPanel{"ObSL"};
        UI::ConsoleLogPanel m_ApplicationLogsPanel{"Logs"};

        UI::UiStream InterpreterStream{m_InterpreterLogsPanel.uiSink};
    };
} // namespace Editor
