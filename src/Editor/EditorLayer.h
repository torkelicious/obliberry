#pragma once
#include <cstdint>
#include <memory>

#include "Core/ApplicationLayer.h"
#include "Editor/UI/InspectorPanel.h"
#include "Editor/UI/RegistryPanel.h"
#include "Editor/UI/ViewportPanel.h"

#include "EditorCamera.h"
#include "Map/HexCoords.h"
#include "Scenes/Scene.h"
#include "Scenes/SceneManager.h"

namespace Editor {
    enum EditorMode : uint8_t { EditorMode, MapEditorMode, PlayMode };

    class EditorLayer : public Core::ApplicationLayer {
    public:
        void Init(Core::EngineContext &ctx) override;

        void Update(float dt) override;

        void Render() override;

        void Shutdown() override;

    private:
        void DrawInterface();

        void DrawDockSpace();

        void DrawEditorPanels();

        void DrawGameView() const;

        void DrawUtilityWindows();

        void DrawToolbar();

        void DrawProjectHub();

        void HandleInput(float dt);

        void LoadScene(const std::string &path);

        void SaveScene();

        Core::EngineContext m_Context;
        Scenes::Scene *m_Scene = nullptr;
        std::string m_CurrentScenePath;
        EditorCamera m_Camera;
        Core::InputManager *m_Input = nullptr;
        ECS::Registry *m_Registry = nullptr;
        Scenes::SceneManager m_SceneManager;

        bool m_Playing = false;

        Map::HexCoords m_SelectedTile;

        // UI
        static bool s_ShouldBuildDock;
        UI::RegistryPanel m_RegistryPanel;
        UI::InspectorPanel m_InspectorPanel;
        UI::ViewportPanel m_ViewportPanel;

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
