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

enum EditorMode : uint8_t { Normal, Map };

class EditorLayer : public ApplicationLayer {
public:
    void Init(EngineContext &ctx) override;

    void Update(float dt) override;

    void Render() override;

    void Shutdown() override;

private:
    void DrawInterface();

    void DrawDockSpace();

    void DrawEditorPanels();

    void DrawGameView();

    void DrawUtilityWindows();

    void HandleInput(float dt);

    void LoadScene(const std::string &path);

    void SaveScene();

    EngineContext m_Context;
    Scene *m_Scene = nullptr;
    std::string m_CurrentScenePath;
    EditorCamera m_Camera;
    InputManager *m_Input = nullptr;
    Registry *m_Registry = nullptr;
    SceneManager m_SceneManager;

    bool m_Playing = false;

    HexCoords m_SelectedTile;

    // UI
    static bool s_ShouldBuildDock;
    RegistryPanel m_RegistryPanel;
    InspectorPanel m_InspectorPanel;
    ViewportPanel m_ViewportPanel;
};
