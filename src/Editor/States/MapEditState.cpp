#include "Rendering/Renderer.h"
#include "Core/Constants.h"
#include "MapEditState.h"
#include "../EditorLayer.h"
#include "Core/InputManager.h"
#include "Core/ResourceManager.h"
#include "ECS/Systems/MapRenderSystem.h"
#include "Math/HexMath.h"

#include <imgui.h>
#include "imgui_internal.h"

void Editor::MapEditState::OnEnter() {

    if (!m_MapState || !m_MapComp) {
        // it is safe to assume each scene only has one map, no less no more.. otherwise something has probably gone
        // seriously wrong
        // TODO: probably enforce a required map ??
        m_MapState = m_EditorLayer->m_Registry->GetFirst<ECS::Components::MapStateComponent>();
        m_MapComp = m_EditorLayer->m_Registry->GetFirst<ECS::Components::MapComponent>();
        m_MapComp->pathToMat->color = {0.0, 0.8, 1.0, 0.50};   // cyan
        m_MapComp->outlineMat->color = {1.0, 0.85, 0.0, 0.55}; // gold / yellowish
        m_CurrentGrid = &m_MapComp->grid;
        m_selectedTile = m_CurrentGrid->Get(m_selectedHex);
        m_TileEditorPanel.SetContext(m_EditorLayer->m_Scene, m_EditorLayer->m_Context);
        m_TileEditorPanel.SetMapComponent(m_MapComp);
        m_TileEditorPanel.SetCreateTypeFn(
                [this](const std::shared_ptr<Rendering::Texture> &tex, const glm::vec4 &color) {
                    return GetOrCreateTypeForMaterial(tex, color);
                });
    }
    if (!m_MapComp->typeMats.empty()) {
        m_TileEditorPanel.InitBrushFromFirstType();
    }
    m_TileEditorPanel.SetSelectedTile(m_selectedTile);
}

void Editor::MapEditState::OnUpdate(const float dt) {}

void Editor::MapEditState::OnHandleInput(const float dt) {

    const bool viewportHovered = m_EditorLayer->m_ViewportPanel.IsHovered();

    if (m_EditorLayer->m_Input->IsKeyPressed("V")) {
        m_EditorLayer->m_Camera.ToggleViewMode();
    }

    // Reset keyboard pan velocity when not hovering
    float kbPanX = 0.0f;
    float kbPanY = 0.0f;
    m_EditorLayer->m_Camera.StopKeyboardPan();

    if (!viewportHovered)
        return;

    m_hoveredHex = Math::HexMath::PixelToHex(m_EditorLayer->m_ViewportPanel.MousePosToWorld(m_EditorLayer->m_Camera));

    m_MapState->hasSelection = true;
    // The component names are misleading: "selectedHex" is actually the hovered hex,
    // while "pathTo" represents the real clicked selection.
    m_MapState->selectedHex = m_hoveredHex;

    // click &/or drag
    if (m_EditorLayer->m_Input->IsMouseDown(0)) {
        ApplyToolAt(m_hoveredHex);
    }

    // Scroll zoom
    if (const auto scrollDelta = static_cast<float>(m_EditorLayer->m_Input->ScrollY()); scrollDelta != 0.0f) {
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
    m_EditorLayer->DrawEditorUI();
    m_TileEditorPanel.OnImGuiRender();

    auto *renderer = m_EditorLayer->m_Context.renderer;
    renderer->BeginFrame();

    const glm::mat4 &vp = renderer->GetCurrentVP();
    const Math::Frustum::ViewFrustum frustum = Math::Frustum::FromCameraVP(vp, Core::HEX_SIZE * 2.0f);

    ECS::Systems::MapRenderSystem::RenderAll(*m_EditorLayer->m_Registry, m_EditorLayer->m_Context, frustum);

    if (m_CurrentTool != Select && m_BrushRadius > 1 && m_MapComp->hexMesh && m_MapComp->outlineMat) {
        ForEachHexInRing(m_hoveredHex, m_BrushRadius - 1, [&](const Map::HexCoords &hex) {
            const auto worldPos = Map::HexGrid::GetWorldPos(hex);
            Rendering::Transform t;
            t.SetPosition({worldPos.x, worldPos.y, 0.01f});
            t.SetScale({1.08f, 1.08f, 1.0f});
            renderer->Submit(m_MapComp->hexMesh, m_MapComp->outlineMat.get(), t);
        });
    }

    renderer->SetLightmap(nullptr);
}

void Editor::MapEditState::OnDrawModeToolbar() {
    {
        constexpr auto activeCol = ImVec4(0.3f, 0.6f, 1.0f, 0.7f);
        constexpr auto inactiveCol = ImVec4(0.2f, 0.2f, 0.2f, 0.5f);

        auto drawToolBtn = [&](const char *label, Tool tool, const char *tip) {
            if (m_CurrentTool == tool) {
                ImGui::PushStyleColor(ImGuiCol_Button, activeCol);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, activeCol);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeCol);
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, inactiveCol);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.5f, 0.9f, 0.6f));
            }
            if (ImGui::Button(label)) {
                m_CurrentTool = tool;
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", tip);
            }
            ImGui::PopStyleColor(3);
        };

        drawToolBtn("  Paint  ", Paint, "Left-click to place tiles. Drag to paint continuously.");
        ImGui::SameLine();
        drawToolBtn("  Erase  ", Erase, "Left-click to remove tiles. Drag to erase continuously.");
        ImGui::SameLine();
        drawToolBtn("  Select ", Select, "Click a tile to inspect and edit its properties.");
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
    ImGui::Text("Size");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    int radius = m_BrushRadius;
    if (ImGui::SliderInt("##Radius", &radius, 1, 5, "%d")) {
        m_BrushRadius = radius;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Brush radius in hexes. Affects Paint and Erase tools.");
    }
}

void Editor::MapEditState::OnDrawUtilityWindows() {}

void Editor::MapEditState::OnExit() {
    m_MapState = nullptr;

    m_EditorLayer->m_Registry->ForEach<ECS::Components::MapStateComponent>(
            [&](ECS::Entity, ECS::Components::MapStateComponent *state) { state->hasSelection = false; });
}

void Editor::MapEditState::ApplyToolAt(const Map::HexCoords &hex) {
    switch (m_CurrentTool) {
        case Select:
            m_selectedHex = hex;
            m_MapState->pathTo = hex;
            m_MapState->hasPathTo = true;
            m_selectedTile = m_CurrentGrid->Get(hex);
            m_TileEditorPanel.SetSelectedTile(m_selectedTile);
            if (m_selectedTile) {
                m_TileEditorPanel.CopyBrushFromTile(*m_selectedTile);
            }
            break;

        case Paint: {
            const uint8_t brushType = m_TileEditorPanel.GetSourceType();
            ForEachHexInRing(hex, m_BrushRadius - 1, [&](const Map::HexCoords &h) {
                m_CurrentGrid->RemoveTileAt(h);
                m_CurrentGrid->EmplaceTile(h, brushType);
            });
            m_MapComp->needsMeshUpdate = true;
            break;
        }

        case Erase:
            ForEachHexInRing(hex, m_BrushRadius - 1, [&](const Map::HexCoords &h) { m_CurrentGrid->RemoveTileAt(h); });
            m_MapComp->needsMeshUpdate = true;
            break;
    }
}

void Editor::MapEditState::ForEachHexInRing(const Map::HexCoords center, const int radius,
                                            const std::function<void(const Map::HexCoords &)> &callback) const {
    const auto [cx, cy, cz] = Math::HexMath::OddRToCube(center);

    for (int dx = -radius; dx <= radius; ++dx) {
        const int dyMin = std::max(-radius, -dx - radius);
        const int dyMax = std::min(radius, -dx + radius);
        for (int dy = dyMin; dy <= dyMax; ++dy) {
            const int dz = -dx - dy;
            const int r = cz + dz;
            const int q = cx + dx + (r - (r & 1)) / 2;
            callback(Map::HexCoords(q, r));
        }
    }
}

uint8_t Editor::MapEditState::GetOrCreateTypeForMaterial(const std::shared_ptr<Rendering::Texture> &tex,
                                                         const glm::vec4 &color) const {
    auto &typeMats = m_MapComp->typeMats;
    for (const auto &[id, mat] : typeMats) {
        if (mat.texture == tex && mat.color == color) {
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
