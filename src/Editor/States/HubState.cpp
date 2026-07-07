#include "HubState.h"
#include "../EditorLayer.h"
#include <iostream>

namespace Editor {

    void HubState::OnEnter() { std::cout << "[Editor] Entering Hub State\n"; }

    void HubState::OnExit() { std::cout << "[Editor] Exiting Hub State\n"; }

    void HubState::OnUpdate(float /*dt*/) {}

    void HubState::OnHandleInput(float /*dt*/) {}

    void HubState::OnDrawPanels() {}

    void HubState::OnRender() { m_EditorLayer->DrawProjectHub(); }

} // namespace Editor
