#include "ECS/Systems/LightingSystem.h"

#include "EditState.h"
#include "../EditorLayer.h"
#include "Core/InputManager.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace Editor {
    ImGuizmo::OPERATION EditState::mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE EditState::mCurrentGizmoMode = ImGuizmo::WORLD;
} // namespace Editor

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

    // Gizmo operation shortcuts
    if (m_EditorLayer->m_Input->IsKeyPressed("T")) {
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    }
    if (m_EditorLayer->m_Input->IsKeyPressed("R")) {
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    }
    if (m_EditorLayer->m_Input->IsKeyPressed("E")) {
        mCurrentGizmoOperation = ImGuizmo::SCALE;
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

void Editor::EditState::OnDrawPanels() {
    m_EditorLayer->m_RegistryPanel.OnImGuiRender();
    m_EditorLayer->m_InspectorPanel.OnImGuiRender();

    // draw gizmo for the currently selected entity (must run after SetDrawlist/SetRect)
    DrawGizmoForSelected();
}

void Editor::EditState::DrawGizmoForSelected() {
    const ECS::Entity selectedEntity = m_EditorLayer->m_RegistryPanel.GetSelectedEntity();
    if (!selectedEntity)
        return;
    if (!selectedEntity.HasComponent<ECS::Components::TransformComponent>())
        return;

    Rendering::Transform &t = selectedEntity.GetComponent<ECS::Components::TransformComponent>()->transform;
    EditTransform(t);
}

void Editor::EditState::EditTransform(Rendering::Transform &transform) {
    const auto &camera = m_EditorLayer->m_Camera;
    const float aspect = m_EditorLayer->m_ViewportPanel.GetWidth() / m_EditorLayer->m_ViewportPanel.GetHeight();

    // use the exact View matrix from the camera
    glm::mat4 gizmoViewMatrix = camera.GetRotation() * camera.GetViewMatrix();
    gizmoViewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -50.0f)) * gizmoViewMatrix;

    const glm::mat4 projMatrix = camera.GetProjectionMatrix(aspect);

    glm::mat4 gizmoMatrix = transform.GetMatrix();
    auto deltaMatrix = glm::mat4(1.0f);

    ImGuizmo::SetOrthographic(true);
    ImGuizmo::Manipulate(
        glm::value_ptr(gizmoViewMatrix),
        glm::value_ptr(projMatrix),
        mCurrentGizmoOperation,
        mCurrentGizmoMode,
        glm::value_ptr(gizmoMatrix),
        glm::value_ptr(deltaMatrix)
    );

    if (ImGuizmo::IsUsing()) {
        if (mCurrentGizmoOperation == ImGuizmo::TRANSLATE) {
            // for translation use the delta directly.
            const auto deltaPos = glm::vec3(deltaMatrix[3]);
            transform.SetPosition(transform.GetPosition() + deltaPos);
        } else {
            // FIXME: probably have to patch more as rotation/scale is still a mess, but translation works.
            float translation[3], rotation[3], scale[3];
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(gizmoMatrix), translation, rotation, scale);

            transform.SetPosition(glm::vec3(translation[0], translation[1], translation[2]));
            // transform stores radians
            transform.SetRotation(
                glm::vec3(glm::radians(rotation[0]), glm::radians(rotation[1]), glm::radians(rotation[2])));
            transform.SetScale(glm::vec3(scale[0], scale[1], scale[2]));
        }
    }
}
