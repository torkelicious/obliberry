#include "ECS/Systems/LightingSystem.h"

#include "MapEditState.h"
#include "../EditorLayer.h"
#include "Core/InputManager.h"

// TODO: IMPLEMENT MAP EDITING FOR REAL
//  MOST OF THIS IS JUST SAME AS EDITSTATE SINCE THIS IS PLACEHOLDER FOR NOW!!!

void Editor::MapEditState::OnUpdate(const float dt) {
    // Same as EditState
    ECS::Systems::LightingSystem::Update(*m_EditorLayer->m_Registry);
}

void Editor::MapEditState::OnHandleInput(const float dt) {
    // Same camera as EditState
    if (m_EditorLayer->m_Input->IsKeyPressed("V")) {
        m_EditorLayer->m_Camera.ToggleViewMode();
    }

    if (!m_EditorLayer->m_ViewportPanel.IsHovered())
        return;

    const auto scrollDelta = static_cast<float>(m_EditorLayer->m_Input->ScrollY());
    if (scrollDelta != 0.0f) {
        m_EditorLayer->m_Camera.AdjustZoom(scrollDelta * 0.2f);
    }

    const auto mouseDeltaX = static_cast<float>(m_EditorLayer->m_Input->GetMouseDeltaX());
    const auto mouseDeltaY = static_cast<float>(m_EditorLayer->m_Input->GetMouseDeltaY());

    if (m_EditorLayer->m_Input->IsMouseDown("MouseMiddle") || m_EditorLayer->m_Input->IsMouseDown("MouseRight")) {
        m_EditorLayer->m_Camera.Pan(-mouseDeltaX, mouseDeltaY, 0.025f);
    }

    float kbPanX = 0.0f;
    float kbPanY = 0.0f;

    if (m_EditorLayer->m_Input->IsKeyDown("W"))
        kbPanY += 1.0f;
    if (m_EditorLayer->m_Input->IsKeyDown("S"))
        kbPanY -= 1.0f;
    if (m_EditorLayer->m_Input->IsKeyDown("A"))
        kbPanX -= 1.0f;
    if (m_EditorLayer->m_Input->IsKeyDown("D"))
        kbPanX += 1.0f;

    if (kbPanX != 0.0f || kbPanY != 0.0f) {
        const float length = std::sqrt(kbPanX * kbPanX + kbPanY * kbPanY);
        kbPanX /= length;
        kbPanY /= length;

        const float speedMod = m_EditorLayer->m_Input->IsKeyDown("LeftShift") ? 3.0f : 1.0f;
        m_EditorLayer->m_Camera.Pan(kbPanX, kbPanY, 15.0f * speedMod * dt);
    }
}

void Editor::MapEditState::OnDrawPanels() {
    const int clickedID = m_EditorLayer->m_ViewportPanel.GetSelectedEntityID();

    if (clickedID != -1) {
        const auto eID = static_cast<ECS::EntityID>(clickedID);

        if (m_EditorLayer->m_Registry->IsValid(eID)) {
            const ECS::Entity selectedEntity(eID, m_EditorLayer->m_Registry);
            m_EditorLayer->m_RegistryPanel.SetSelectedEntity(selectedEntity);
        }
    }

    m_EditorLayer->m_ViewportPanel.ClearSelectedEntityID();

    m_EditorLayer->m_RegistryPanel.OnImGuiRender();
    m_EditorLayer->m_InspectorPanel.OnImGuiRender();
}
