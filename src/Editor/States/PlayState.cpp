#include "PlayState.h"
#include "../EditorLayer.h"
#include "Core/LoggerService.h"


#pragma push_macro("LOG_WHO")
#define LOG_WHO "PlayState"

void Editor::PlayState::OnEnter() {
    m_EditorLayer->m_PendingSceneToLoad.clear();
    m_EditorLayer->m_Camera.SaveState();
    m_EditorLayer->m_ViewportPanel.SetPlayModeIndicator(true);
    LOG_INFO(LOG_WHO, "Entering Play Mode");
    m_EditorLayer->LoadScene(m_EditorLayer->m_CurrentScenePath);
}

void Editor::PlayState::OnExit() {
    m_EditorLayer->m_PendingSceneToLoad.clear();
    m_EditorLayer->m_ViewportPanel.SetPlayModeIndicator(false);
    LOG_INFO(LOG_WHO, "Exiting Play Mode, restoring scene state...");
    m_EditorLayer->LoadScene(m_EditorLayer->m_CurrentScenePath);
    m_EditorLayer->m_Camera.RestoreState();
}

void Editor::PlayState::OnUpdate(const float dt) { m_EditorLayer->m_SceneManager.Update(dt); }

void Editor::PlayState::OnHandleInput(float dt) {
    // handled by the game scene
}

void Editor::PlayState::OnDrawPanels() {
    // nothing drawn
}

void Editor::PlayState::OnRender() { m_EditorLayer->DrawEditorLayout(); }
#pragma pop_macro("LOG_WHO")
