#include "PlayState.h"
#include "../EditorLayer.h"
#include <iostream>

void Editor::PlayState::OnEnter() {
    m_EditorLayer->m_PendingSceneToLoad.clear();
    m_EditorLayer->m_Camera.SaveState();
    m_EditorLayer->m_ViewportPanel.SetPlayModeIndicator(true);
    std::cout << "[Editor] Entering Play Mode\n";
    m_EditorLayer->LoadScene(m_EditorLayer->m_CurrentScenePath);
}

void Editor::PlayState::OnExit() {
    m_EditorLayer->m_PendingSceneToLoad.clear();
    m_EditorLayer->m_ViewportPanel.SetPlayModeIndicator(false);
    std::cout << "[Editor] Exiting Play Mode, restoring scene state...\n";
    m_EditorLayer->LoadScene(m_EditorLayer->m_CurrentScenePath);
    m_EditorLayer->m_Camera.RestoreState();
}

void Editor::PlayState::OnUpdate(const float dt) {
    m_EditorLayer->m_SceneManager.Update(dt);
}

void Editor::PlayState::OnHandleInput(float dt) {
    // handled by the game scene
}

void Editor::PlayState::OnDrawPanels() {
    // nothing drawn
}
