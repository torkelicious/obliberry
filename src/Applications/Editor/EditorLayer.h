#pragma once
#include "EditorContext.h"
#include <memory>
#include <sstream>
#include "Core/ApplicationLayer.h"
#include "Platform/Input/InputManager.h"
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
#include "UI/ConfigWindows/ThemeConfigEditor.h"

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

        void SetupFontSync(std::atomic<bool> *fontsDirty) override;

        void PreImGuiFrame() override;

        void SyncFonts(Core::EngineContext &ctx, std::mutex &imguiTextureMutex) override;

        void Update(float dt) override;

        void Render() override;

        void Shutdown() override;

        EditorContext *GetEditorContext() { return &m_EditorContext; }

        inline static bool s_ShouldBuildDock = true;

        inline static bool s_RenderParticlesInEditor = false;

        void ReloadCurrentMap() const;

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
        bool m_ShowSceneConfig = false;
        bool m_ShowProjectConfig = false;
        bool m_ShowGraphicsConfig = false;
        bool m_ShowThemeConfig = false;

        // commands
        Commands::UndoManager m_UndoManager;

        // Logging
        static constexpr size_t MAX_CONSOLE_LINES = 2000;
        std::vector<std::string> m_ConsoleLogs;
        size_t m_PreviousLogCount = 0;
        std::stringstream m_InterpreterOutput;

        void FlushInterpreterOutput() {
            std::string line;
            while (std::getline(m_InterpreterOutput, line)) {
                m_ConsoleLogs.push_back(std::move(line));
                if (m_ConsoleLogs.size() > MAX_CONSOLE_LINES)
                    m_ConsoleLogs.erase(m_ConsoleLogs.begin(), m_ConsoleLogs.end() - MAX_CONSOLE_LINES);
            }
            m_InterpreterOutput.clear();
        }
    };
} // namespace Editor
