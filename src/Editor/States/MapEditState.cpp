#include "Rendering/Renderer.h"
#include "Core/Constants.h"
#include "MapEditState.h"
#include "../EditorLayer.h"
#include "Core/InputManager.h"
#include "Core/ResourceManager.h"
#include "ECS/Systems/MapRenderSystem.h"

#include <imgui.h>
#include <ImGuizmo.h>

// TODO: IMPLEMENT MAP EDITING FOR REAL
//  MOST OF THIS IS JUST SAME AS EDITSTATE SINCE THIS IS PLACEHOLDER FOR NOW!!!

void Editor::MapEditState::OnEnter() {

    if (!m_MapState || !m_MapComp) {
        // it is safe to assume each scene only has one map, no less no more.. otherwise something has probably gone
        // seriously wrong
        m_MapState = m_EditorLayer->m_Registry->GetFirst<ECS::Components::MapStateComponent>();
        m_MapComp = m_EditorLayer->m_Registry->GetFirst<ECS::Components::MapComponent>();
        m_MapComp->pathToMat->color = {1, 1, 1, 0.25};
        m_MapComp->outlineMat->color = {0.05, 0.0, 0, 0.15};
        m_CurrentGrid = &m_MapComp->grid;
    }
}

void Editor::MapEditState::OnUpdate(const float dt) {}

void Editor::MapEditState::OnHandleInput(const float dt) {
    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    if (m_EditorLayer->m_Input->IsKeyPressed("V")) {
        m_EditorLayer->m_Camera.ToggleViewMode();
    }

    // Reset keyboard pan velocity when not hovering
    float kbPanX = 0.0f;
    float kbPanY = 0.0f;
    m_EditorLayer->m_Camera.StopKeyboardPan();

    if (!m_EditorLayer->m_ViewportPanel.IsHovered())
        return;

    // Click input
    if (m_EditorLayer->m_Input->IsMouseDown(0)) {
        ToolClickEvent();
    }

    // Hex hover
    const glm::vec2 worldPos = m_EditorLayer->m_ViewportPanel.MousePosToWorld(m_EditorLayer->m_Camera);
    m_hoveredHex = Math::HexMath::PixelToHex(worldPos);

    m_MapState->hasSelection = true;
    // The component names are misleading: "selectedHex" is actually the hovered hex,
    // while "pathTo" represents the real clicked selection.
    m_MapState->selectedHex = m_hoveredHex;

    // Scroll zoom
    const auto scrollDelta = static_cast<float>(m_EditorLayer->m_Input->ScrollY());
    if (scrollDelta != 0.0f) {
        m_EditorLayer->m_Camera.AdjustZoom(scrollDelta * 0.2f);
    }

    // Mouse pan
    const auto mouseDeltaX = static_cast<float>(m_EditorLayer->m_Input->GetMouseDeltaX());
    const auto mouseDeltaY = static_cast<float>(m_EditorLayer->m_Input->GetMouseDeltaY());

    if (m_EditorLayer->m_Input->IsMouseDown("MouseMiddle") || m_EditorLayer->m_Input->IsMouseDown("MouseRight")) {
        m_EditorLayer->m_Camera.Pan(-mouseDeltaX, mouseDeltaY, 0.025f);
    }

    // Keyboard pan
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
    }

    const float speedMod = m_EditorLayer->m_Input->IsKeyDown("LeftShift") ? 3.0f : 1.0f;
    constexpr float moveAmount = 3.0f;
    const float vpHeight = m_EditorLayer->m_ViewportPanel.GetHeight();
    m_EditorLayer->m_Camera.KeyboardPan(kbPanX, kbPanY, 15.0f * speedMod * moveAmount * (600.0f / vpHeight));
}

void Editor::MapEditState::OnDrawPanels() {}

void Editor::MapEditState::OnRender() {
    ImGuizmo::BeginFrame();
    m_EditorLayer->DrawEditorUI();

    auto *renderer = m_EditorLayer->m_Context.renderer;
    renderer->BeginFrame();

    const glm::mat4 &vp = renderer->GetCurrentVP();
    const Math::Frustum::ViewFrustum frustum = Math::Frustum::FromCameraVP(vp, Core::HEX_SIZE * 2.0f);

    ECS::Systems::MapRenderSystem::RenderAll(*m_EditorLayer->m_Registry, m_EditorLayer->m_Context, frustum);
    renderer->SetLightmap(nullptr);
}
void Editor::MapEditState::OnDrawModeToolbar() {
    const char *modeItems[] = {"Paint", "Erase", "Select"};
    int current = static_cast<int>(m_CurrentTool);
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::Combo("##MapEditTool", &current, modeItems, IM_ARRAYSIZE(modeItems))) {
        m_CurrentTool = static_cast<Tool>(current);
    }
}

void Editor::MapEditState::OnDrawUtilityWindows() {}
void Editor::MapEditState::OnExit() {
    m_MapState = nullptr;
    m_EditorLayer->m_Registry->ForEach<ECS::Components::MapStateComponent>(
            [&](ECS::Entity, ECS::Components::MapStateComponent *state) { state->hasSelection = false; });
}

void Editor::MapEditState::ToolClickEvent(const int btn) {
    if (btn == 0) {
        m_selectedHex = m_hoveredHex;
        m_MapState->pathTo = m_selectedHex; // stupid naming strikes again! this is still a hack!
        m_MapState->hasPathTo = true;

        switch (m_CurrentTool) {
            case Select:
                break;
            case Paint:
                // emplace tile
                break;
            case Erase:
                // remove tile
                break;
        }
    }
}

uint8_t Editor::MapEditState::GetOrCreateTypeForTexture(const std::shared_ptr<Rendering::Texture> &tex,
                                                        const glm::vec4 &color) const {
    auto &typeMats = m_MapComp->typeMats;
    for (const auto &[id, mat] : typeMats) {
        if (mat.texture == tex) {
            return id;
        }
    }
    // allocate newid
    uint8_t newId = 0;
    while (typeMats.contains(newId)) {
        newId++;
    }

    const auto shader = !typeMats.empty() ? typeMats.begin()->second.shader
                                          : m_EditorLayer->m_Context.resources->Get<Rendering::Shader>("base_shader");

    typeMats.emplace(newId, Rendering::Material{shader, tex, color});
    m_MapComp->needsMeshUpdate = true;
    return newId;
}
