#include "ECS/Systems/LightingSystem.h"

#include "EditState.h"
#include "../EditorLayer.h"
#include "Core/InputManager.h"
#include <glm/gtc/type_ptr.hpp>

void Editor::EditState::OnUpdate(const float dt) {
    // Editor mode only runs lighting for accurate scene view
    // full scene update is for play
    ECS::Systems::LightingSystem::Update(*m_EditorLayer->m_Registry);

    // Entity picking
    if (const int clickedID = m_EditorLayer->m_ViewportPanel.GetSelectedEntityID(); clickedID != -1) {
        const auto eID = static_cast<ECS::EntityID>(clickedID);

        if (m_EditorLayer->m_Registry->IsValid(eID)) {
            const ECS::Entity selectedEntity(eID, m_EditorLayer->m_Registry);
            m_EditorLayer->m_RegistryPanel.SetSelectedEntity(selectedEntity);
        }
    }
    m_EditorLayer->m_ViewportPanel.ClearSelectedEntityID();
}

void Editor::EditState::OnHandleInput(const float dt) {
    if (m_EditorLayer->m_Input->IsKeyPressed("V")) {
        m_EditorLayer->m_Camera.ToggleViewMode();
    }

    // camera controls require viewport hover, because i dont want to steal input from gui interaction!!!
    if (!m_EditorLayer->m_ViewportPanel.IsHovered())
        return;

    // scroll zoom
    const auto scrollDelta = static_cast<float>(m_EditorLayer->m_Input->ScrollY());
    if (scrollDelta != 0.0f) {
        m_EditorLayer->m_Camera.AdjustZoom(scrollDelta * 0.2f);
    }

    // mouse pan
    const auto mouseDeltaX = static_cast<float>(m_EditorLayer->m_Input->GetMouseDeltaX());
    const auto mouseDeltaY = static_cast<float>(m_EditorLayer->m_Input->GetMouseDeltaY());

    if (m_EditorLayer->m_Input->IsMouseDown("MouseMiddle") || m_EditorLayer->m_Input->IsMouseDown("MouseRight")) {
        m_EditorLayer->m_Camera.Pan(-mouseDeltaX, mouseDeltaY, 0.025f);
    }

    // Keyboard pan (WASD)
    float kbPanX = 0.0f;
    float kbPanY = 0.0f;

    if (m_EditorLayer->m_Input->IsKeyDown("W")) kbPanY += 1.0f;
    if (m_EditorLayer->m_Input->IsKeyDown("S")) kbPanY -= 1.0f;
    if (m_EditorLayer->m_Input->IsKeyDown("A")) kbPanX -= 1.0f;
    if (m_EditorLayer->m_Input->IsKeyDown("D")) kbPanX += 1.0f;

    if (kbPanX != 0.0f || kbPanY != 0.0f) {
        const float length = std::sqrt(kbPanX * kbPanX + kbPanY * kbPanY);
        kbPanX /= length;
        kbPanY /= length;
        const float speedMod = m_EditorLayer->m_Input->IsKeyDown("LeftShift") ? 3.0f : 1.0f;
        m_EditorLayer->m_Camera.Pan(kbPanX, kbPanY, 15.0f * speedMod * dt);
    }
}

void Editor::EditState::OnDrawPanels() {
    // Entity inspector / registry panels
    m_EditorLayer->m_RegistryPanel.OnImGuiRender();
    m_EditorLayer->m_InspectorPanel.OnImGuiRender();

    // Draw gizmo for the currently selected entity (must run after ViewportPanel's SetDrawlist/SetRect)
    DrawGizmoForSelected();
}

void Editor::EditState::DrawGizmoForSelected() {
    const ECS::Entity selectedEntity = m_EditorLayer->m_RegistryPanel.GetSelectedEntity();
    if (!selectedEntity) return;
    if (!selectedEntity.HasComponent<ECS::Components::TransformComponent>()) return;

    Rendering::Transform &t = selectedEntity.GetComponent<ECS::Components::TransformComponent>()->transform;
    EditTransform(t);
}

void Editor::EditState::EditTransform(Rendering::Transform &transform) {
    const auto &camera = m_EditorLayer->m_Camera;
    const float aspect = m_EditorLayer->aspect;
    const glm::mat4 viewMatrix = camera.GetRotation() * camera.GetViewMatrix();

    // TODO: implement
    // i tried before but it broken, removing to push other changes cleanly.
    // :(
}
