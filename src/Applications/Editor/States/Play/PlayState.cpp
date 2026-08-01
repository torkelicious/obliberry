#include "PlayState.h"
#include "Applications/Editor/EditorLayer.h"
#include "Logger/LoggerService.h"


#pragma push_macro("LOG_WHO")
#define LOG_WHO "PlayState"

void Editor::States::PlayState::OnEnter() {
    m_EditorLayer->m_Camera.SaveState();
    m_EditorLayer->m_ViewportPanel.SetPlayModeIndicator(true);
    m_EditorLayer->m_Context.isEditorMode = false;
    LOG_INFO(LOG_WHO, "Entering Play Mode");
    m_EntryScenePath = m_EditorLayer->m_CurrentScenePath;
    m_EditorLayer->LoadScene(m_EditorLayer->m_CurrentScenePath);
}

void Editor::States::PlayState::OnExit() {
    m_EditorLayer->m_ViewportPanel.SetPlayModeIndicator(false);
    m_EditorLayer->m_Context.isEditorMode = true;
    LOG_INFO(LOG_WHO, "Exiting Play Mode, restoring scene state...");
    const std::string restorePath = m_EntryScenePath.empty() ? m_EditorLayer->m_CurrentScenePath : m_EntryScenePath;
    m_EditorLayer->LoadScene(restorePath);
    m_EditorLayer->m_Camera.RestoreState();
}

void Editor::States::PlayState::OnUpdate(const float dt) { m_EditorLayer->m_SceneManager.Update(dt); }

void Editor::States::PlayState::OnHandleInput(float dt) {
    // handled by the game scene
}

void Editor::States::PlayState::OnDrawPanels() {
    // nothing drawn
}

void Editor::States::PlayState::OnRender() { m_EditorLayer->DrawEditorLayout(); }
#pragma pop_macro("LOG_WHO")
