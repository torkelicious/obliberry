#include "HubState.h"
#include "../EditorLayer.h"
#include "Core/LoggerService.h"

namespace Editor {


    constexpr auto LOG_WHO = "HubState";

    void HubState::OnEnter() { LOG_INFO(LOG_WHO, "Entering Hub State"); }

    void HubState::OnExit() { LOG_INFO(LOG_WHO, "Exiting Hub State"); }

    void HubState::OnUpdate(float /*dt*/) {}

    void HubState::OnHandleInput(float /*dt*/) {}

    void HubState::OnDrawPanels() {}

    void HubState::OnRender() { m_EditorLayer->DrawProjectHub(); }

} // namespace Editor
