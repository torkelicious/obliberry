#include "ECS/Systems/LightingSystem.h"

#include "EditState.h"
#include "../EditorLayer.h"
#include "Core/InputManager.h"

void Editor::EditState::OnUpdate(EditorLayer &editor, const float dt) {
    // Editor mode only runs lighting for accurate scene view
    // full scene update is for play
    ECS::Systems::LightingSystem::Update(*editor.m_Registry);
}

void Editor::EditState::OnHandleInput(EditorLayer &editor, const float dt) {
    if (editor.m_Input->IsKeyPressed("V")) {
        editor.m_Camera.ToggleViewMode();
    }

    // camera controls require viewport hover, because i dont want to steal input from gui interaction!!!
    if (!editor.m_ViewportPanel.IsHovered())
        return;

    // scroll zoom
    const auto scrollDelta = static_cast<float>(editor.m_Input->ScrollY());
    if (scrollDelta != 0.0f) {
        editor.m_Camera.AdjustZoom(scrollDelta * 0.2f);
    }

    // mouse pan
    const auto mouseDeltaX = static_cast<float>(editor.m_Input->GetMouseDeltaX());
    const auto mouseDeltaY = static_cast<float>(editor.m_Input->GetMouseDeltaY());

    if (editor.m_Input->IsMouseDown("MouseMiddle") || editor.m_Input->IsMouseDown("MouseRight")) {
        editor.m_Camera.Pan(-mouseDeltaX, mouseDeltaY, 0.025f);
    }

    // Keyboard pan (WASD)
    float kbPanX = 0.0f;
    float kbPanY = 0.0f;

    if (editor.m_Input->IsKeyDown("W")) kbPanY += 1.0f;
    if (editor.m_Input->IsKeyDown("S")) kbPanY -= 1.0f;
    if (editor.m_Input->IsKeyDown("A")) kbPanX -= 1.0f;
    if (editor.m_Input->IsKeyDown("D")) kbPanX += 1.0f;

    if (kbPanX != 0.0f || kbPanY != 0.0f) {
        const float length = std::sqrt(kbPanX * kbPanX + kbPanY * kbPanY);
        kbPanX /= length;
        kbPanY /= length;
        const float speedMod = editor.m_Input->IsKeyDown("LeftShift") ? 3.0f : 1.0f;
        editor.m_Camera.Pan(kbPanX, kbPanY, 15.0f * speedMod * dt);
    }
}

void Editor::EditState::OnDrawPanels(EditorLayer &editor) {
    // Entity picking
    const int clickedID = editor.m_ViewportPanel.GetSelectedEntityID();

    if (clickedID != -1) {
        const auto eID = static_cast<ECS::EntityID>(clickedID);

        if (editor.m_Registry->IsValid(eID)) {
            const ECS::Entity selectedEntity(eID, editor.m_Registry);
            editor.m_RegistryPanel.SetSelectedEntity(selectedEntity);
        }
    }
    editor.m_ViewportPanel.ClearSelectedEntityID();
    // Entity inspector / registry panels
    editor.m_RegistryPanel.OnImGuiRender();
    editor.m_InspectorPanel.OnImGuiRender();
}
