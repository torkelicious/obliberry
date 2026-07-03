#include "PlayState.h"
#include "../EditorLayer.h"
#include <iostream>

void Editor::PlayState::OnEnter(EditorLayer &editor) {
    editor.m_PendingSceneToLoad.clear();
    editor.m_Camera.SaveState();
    editor.m_ViewportPanel.SetPlayModeIndicator(true);
    std::cout << "[Editor] Entering Play Mode\n";
    editor.LoadScene(editor.m_CurrentScenePath);
}

void Editor::PlayState::OnExit(EditorLayer &editor) {
    editor.m_PendingSceneToLoad.clear();
    editor.m_ViewportPanel.SetPlayModeIndicator(false);
    std::cout << "[Editor] Exiting Play Mode, restoring scene state...\n";
    editor.LoadScene(editor.m_CurrentScenePath);
    editor.m_Camera.RestoreState();
}

void Editor::PlayState::OnUpdate(EditorLayer &editor, const float dt) {
    editor.m_SceneManager.Update(dt);
}

void Editor::PlayState::OnHandleInput(EditorLayer &, float) {
    // handled by the game scene
}

void Editor::PlayState::OnDrawPanels(EditorLayer &) {
    // nothing drawn
}
