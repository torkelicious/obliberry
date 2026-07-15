#include "HubState.h"
#include "Applications/Editor/EditorLayer.h"
#include "Logger/LoggerService.h"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "HubState"

namespace Editor::States {

    void HubState::OnEnter() { LOG_INFO(LOG_WHO, "Entering Hub State"); }

    void HubState::OnExit() { LOG_INFO(LOG_WHO, "Exiting Hub State"); }

    void HubState::OnUpdate(float /*dt*/) {}

    void HubState::OnHandleInput(float /*dt*/) {}

    void HubState::OnDrawPanels() {}

    void HubState::OnRender() { m_EditorLayer->DrawProjectHub(); }

} // namespace Editor::States
#pragma pop_macro("LOG_WHO")
