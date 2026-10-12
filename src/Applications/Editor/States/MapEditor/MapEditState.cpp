#include "MapEditState.h"

#include "Applications/Editor/States/MapEditor/MapTools.h"
#include "Map/HexCoords.h"
#include "Rendering/Renderer.h"
#include "Core/Constants.h"
#include "Applications/Editor/EditorLayer.h"
#include "Platform/Input/InputManager.h"
#include "Core/ResourceManager.h"
#include "ECS/Systems/MapRenderSystem.h"
#include "Math/HexMath.h"
#include "Applications/Editor/Platform/FileDialogs.h"
#include "Applications/Editor/Commands/EditorCommands.h"
#include "IO/MapSerialization.h"
#include "IO/VFS/VFS.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <utility>

#include <imgui.h>
#include "imgui_internal.h"

namespace {
    constexpr auto kActiveCol = ImVec4(0.3f, 0.6f, 1.0f, 0.7f);
    constexpr auto kInactiveCol = ImVec4(0.2f, 0.2f, 0.2f, 0.5f);
    constexpr auto kInactiveHoverCol = ImVec4(0.3f, 0.3f, 0.3f, 0.6f);
    constexpr auto kInactivePressedCol = ImVec4(0.25f, 0.5f, 0.9f, 0.6f);

    constexpr float kPreviewZ = 0.01f;
    constexpr float kSelectionZ = 0.1f;
    constexpr float kOverlayScale = 1.08f;

    std::string GetMapsDirectory() {
        const std::string dir = IO::VFS::Resolve(std::string(Core::MAP_PATH)).string();

        std::filesystem::create_directories(dir);
        return dir;
    }

    bool ToggleButton(const char *label, const bool active, const char *tooltip) {
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button, kActiveCol);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kActiveCol);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, kActiveCol);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, kInactiveCol);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kInactiveHoverCol);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, kInactivePressedCol);
        }
        const bool clicked = ImGui::Button(label);
        ImGui::PopStyleColor(3);

        if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", tooltip);
        }
        return clicked;
    }

    void ToolbarSeparator() {
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
    }

    constexpr int HexCountForSize(const int size) { return 3 * size * (size - 1) + 1; }
} // namespace

#pragma push_macro("LOG_WHO")
#define LOG_WHO "MapEditState"

namespace Editor::States {

    //
    // Lifecycle
    //

    void MapEditState::OnEnter() {
        m_MapState = m_EditorLayer->m_Registry->GetFirst<ECS::Components::MapStateComponent>();
        m_MapComp = m_EditorLayer->m_Registry->GetFirst<ECS::Components::MapComponent>();
        m_CurrentGrid = m_MapComp ? &m_MapComp->grid : nullptr;

        ResetTransientState();

        if (!m_MapComp) {
            LOG_ERROR(LOG_WHO, "No MapComponent found in scene, cannot enter map editor");
            return;
        }

        ApplyOverlayColors();

        m_TileEditorPanel.SetContext(m_EditorLayer->m_Scene, *m_EditorLayer->m_Context);
        m_TileEditorPanel.SetMapComponent(m_MapComp);
        m_TileEditorPanel.SetCreateTypeFn([this](const std::shared_ptr<Rendering::Texture> &tex, const glm::vec4 &color) { return GetOrCreateTypeForMaterial(tex, color); });

        if (!m_MapComp->typeMats.empty()) {
            m_TileEditorPanel.InitBrushFromFirstType();
        }

        m_TileEditorPanel.SetMapTool(m_CurrentTool);

        SetWindowTitle(m_MapComp->mapFilePath);
    }

    void MapEditState::OnUpdate(const float dt) { SyncSelection(); }

    void MapEditState::OnDrawPanels() {}

    void MapEditState::OnDrawUtilityWindows() {}

    void MapEditState::OnExit() {
        CancelMapDrag();
        ClearSelection();

        m_MapState = nullptr;
        m_MapComp = nullptr;
        m_CurrentGrid = nullptr;

        m_WasMouseDown = false;
        m_PreDragState.clear();
        m_PostDragState.clear();

        m_EditorLayer->m_Registry->ForEach<ECS::Components::MapStateComponent>([&](ECS::Entity, ECS::Components::MapStateComponent *state) { state->hasSelection = false; });
    }

    void MapEditState::OnSceneLoaded() {
        m_MapState = m_EditorLayer->m_Registry->GetFirst<ECS::Components::MapStateComponent>();
        m_MapComp = m_EditorLayer->m_Registry->GetFirst<ECS::Components::MapComponent>();
        m_CurrentGrid = m_MapComp ? &m_MapComp->grid : nullptr;

        ResetTransientState();

        if (m_MapComp) {
            ApplyOverlayColors();
            m_TileEditorPanel.SetMapComponent(m_MapComp);
            if (!m_MapComp->typeMats.empty()) {
                m_TileEditorPanel.InitBrushFromFirstType();
            }
        }
    }

    void MapEditState::ResetTransientState() {
        m_MapDragging = false;
        m_WasMouseDown = false;
        m_LastStampHex.reset();
        m_PreDragState.clear();
        m_PostDragState.clear();
        m_SelectionAtDragStart.clear();
        ClearSelection();
    }

    void MapEditState::ApplyOverlayColors() const {
        if (!m_MapComp) {
            return;
        }
        if (m_MapComp->pathToMat) {
            m_MapComp->pathToMat->color = {0.0f, 0.8f, 1.0f, 0.50f};
        }
        if (m_MapComp->outlineMat) {
            m_MapComp->outlineMat->color = {1.0f, 0.85f, 0.0f, 0.55f};
        }
    }

    //
    // Input
    //

    void MapEditState::OnHandleInput(const float dt) {
        auto &input = *m_EditorLayer->m_Input;

        const bool mouseDown = input.IsMouseDown(0);
        const bool mousePressed = mouseDown && !m_WasMouseDown;
        m_WasMouseDown = mouseDown;

        if (!input.HasAnyModifierDown() && input.IsKeyPressed("V")) {
            m_EditorLayer->m_Camera.ToggleViewMode();
        }

        m_EditorLayer->m_Camera.StopKeyboardPan();

        if (m_MapDragging && input.IsKeyPressed("Escape")) {
            CancelMapDrag();
        }

        if (!m_EditorLayer->m_ViewportPanel.IsHovered()) {
            if (m_MapState) {
                m_MapState->hasSelection = false;
            }
            if (input.IsMouseReleased(0)) {
                FinishMapDrag();
            }
            return;
        }

        HandleCameraInput();

        m_HoveredHex = Math::HexMath::PixelToHex(m_EditorLayer->m_ViewportPanel.MousePosToWorld(m_EditorLayer->m_Camera));

        if (m_MapState) {
            m_MapState->hasSelection = true;
            m_MapState->selectedHex = m_HoveredHex;
        }

        if (!m_MapState || !m_MapComp || !m_CurrentGrid) {
            if (input.IsMouseReleased(0)) {
                FinishMapDrag();
            }
            return;
        }

        // tool hotkeys
        if (!m_MapDragging && !input.HasAnyModifierDown()) {
            if (input.IsKeyPressed("B")) {
                SetTool(MapTool::Paint);
            } else if (input.IsKeyPressed("E")) {
                SetTool(MapTool::Erase);
            } else if (input.IsKeyPressed("Q")) {
                SetTool(MapTool::Select);
            }
        }

        if (mousePressed && !m_MapDragging) {
            BeginMapDrag(m_HoveredHex);
        }

        if (m_MapDragging && mouseDown) {
            UpdateMapDrag(m_HoveredHex);
        }

        if (input.IsMouseReleased(0) && m_MapDragging) {
            UpdateMapDrag(m_HoveredHex); // pick up the final position
            FinishMapDrag();
        }
    }

    void MapEditState::HandleCameraInput() {
        auto &input = *m_EditorLayer->m_Input;

        // scroll: zoom, or brush size with ctrl held
        if (const auto scrollDelta = static_cast<float>(input.ScrollY()); scrollDelta != 0.0f) {
            if (input.HasCtrlDown() && m_ToolShape == MapToolShape::RADIALBRUSH) {
                m_BrushSize = std::clamp(m_BrushSize + (scrollDelta > 0.0f ? 1 : -1), 1, kMaxBrushSize);
            } else {
                m_EditorLayer->m_Camera.AdjustZoom(scrollDelta * 0.2f);
            }
        }

        // mouse pan
        const auto mouseDeltaX = static_cast<float>(input.GetMouseDeltaX());
        const auto mouseDeltaY = static_cast<float>(input.GetMouseDeltaY());

        if (input.IsMouseDown("MouseMiddle") || input.IsMouseDown("MouseRight")) {
            m_EditorLayer->m_Camera.Pan(-mouseDeltaX, mouseDeltaY, 0.025f);
        }

        // keyboard pan
        float kbPanX = 0.0f;
        float kbPanY = 0.0f;

        if (!input.HasCtrlDown() && !input.HasAltDown()) {
            if (input.IsKeyDown("W")) {
                kbPanY += 1.0f;
            }
            if (input.IsKeyDown("S")) {
                kbPanY -= 1.0f;
            }
            if (input.IsKeyDown("A")) {
                kbPanX -= 1.0f;
            }
            if (input.IsKeyDown("D")) {
                kbPanX += 1.0f;
            }
        }

        if (kbPanX != 0.0f || kbPanY != 0.0f) {
            const float length = std::sqrt(kbPanX * kbPanX + kbPanY * kbPanY);
            kbPanX /= length;
            kbPanY /= length;
        }

        const float speedMod = input.IsKeyDown("LeftShift") ? 3.0f : 1.0f;
        constexpr float moveAmount = 3.0f;
        const float vpHeight = m_EditorLayer->m_ViewportPanel.GetHeight();
        if (vpHeight > 0.0f) {
            m_EditorLayer->m_Camera.KeyboardPan(kbPanX, kbPanY, 15.0f * speedMod * moveAmount * (600.0f / vpHeight));
        }
    }

    //
    // Shapes
    //

    void MapEditState::ForEachHexInShape(const Map::HexCoords &anchor, const Map::HexCoords &cursor, const HexCallback &callback) const {
        switch (m_ToolShape) {
            case MapToolShape::RADIALBRUSH:
                ForEachHexWithinRadius(cursor, BrushRings(), callback);
                break;
            case MapToolShape::RECT:
                ForEachHexInRect(anchor, cursor, callback);
                break;
            case MapToolShape::CIRC:
                ForEachHexInCircle(anchor, cursor, callback);
                break;
        }
    }

    void MapEditState::ForEachHexWithinRadius(const Map::HexCoords center, const int radius, const HexCallback &callback) {
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

    void MapEditState::ForEachHexInRect(const Map::HexCoords &start, const Map::HexCoords &end, const HexCallback &callback) {
        const int minCol = std::min(start.q, end.q);
        const int maxCol = std::max(start.q, end.q);
        const int minRow = std::min(start.r, end.r);
        const int maxRow = std::max(start.r, end.r);

        for (int row = minRow; row <= maxRow; ++row) {
            for (int col = minCol; col <= maxCol; ++col) {
                callback(Map::HexCoords(col, row));
            }
        }
    }

    void MapEditState::ForEachHexInCircle(const Map::HexCoords &center, const Map::HexCoords &edge, const HexCallback &callback) {
        const glm::vec2 centerPos = Map::HexGrid::GetWorldPos(center);
        const glm::vec2 delta = Map::HexGrid::GetWorldPos(edge) - centerPos;
        const float radiusSquared = delta.x * delta.x + delta.y * delta.y;
        const float radius = std::sqrt(radiusSquared);

        const glm::vec2 origin = Map::HexGrid::GetWorldPos(Map::HexCoords(0, 0));
        const float colStep = std::abs(Map::HexGrid::GetWorldPos(Map::HexCoords(1, 0)).x - origin.x);
        const float rowStep = std::abs(Map::HexGrid::GetWorldPos(Map::HexCoords(0, 1)).y - origin.y);

        const int colRadius = static_cast<int>(std::ceil(radius / colStep)) + 1;
        const int rowRadius = static_cast<int>(std::ceil(radius / rowStep)) + 1;

        for (int row = center.r - rowRadius; row <= center.r + rowRadius; ++row) {
            for (int col = center.q - colRadius; col <= center.q + colRadius; ++col) {
                const Map::HexCoords hex(col, row);
                const glm::vec2 offset = Map::HexGrid::GetWorldPos(hex) - centerPos;

                if (offset.x * offset.x + offset.y * offset.y <= radiusSquared + 1e-4f) {
                    callback(hex);
                }
            }
        }
    }

    //
    // Tool switching and strokes
    //

    void MapEditState::SetTool(const MapTool tool) {
        if (tool == m_CurrentTool) {
            return;
        }
        CancelMapDrag();
        if (m_CurrentTool == MapTool::Select) {
            ClearSelection();
        }
        m_CurrentTool = tool;
        m_TileEditorPanel.SetMapTool(tool);
    }

    void MapEditState::BeginMapDrag(const Map::HexCoords &hex) {
        auto &input = *m_EditorLayer->m_Input;

        m_MapDragging = true;
        m_PreDragState.clear();
        m_PostDragState.clear();
        m_DragStartHex = hex;
        m_DragEndHex = hex;
        m_LastStampHex.reset();

        if (m_CurrentTool == MapTool::Select) {
            m_SelectMode = input.HasCtrlDown() ? SelectMode::Subtract : (input.IsKeyDown("LeftShift") ? SelectMode::Add : SelectMode::Replace);
            m_SelectionAtDragStart = m_Selection;

            if (m_SelectMode == SelectMode::Replace && !IsDragShape()) {
                ClearSelection();
            }
        } else if (m_MapState) {
            m_MapState->hasPathTo = false;
        }

        if (!IsDragShape()) {
            StampBrushAlongStroke(hex);
        }
    }

    void MapEditState::UpdateMapDrag(const Map::HexCoords &hex) {
        m_DragEndHex = hex;
        if (!IsDragShape()) {
            StampBrushAlongStroke(hex);
        }
    }

    void MapEditState::FinishMapDrag() {
        if (!m_MapDragging) {
            return;
        }

        if (IsDragShape()) {
            if (m_CurrentTool == MapTool::Select && m_SelectMode == SelectMode::Replace) {
                m_Selection.clear();
                m_PrimaryHex.reset();
            }
            ForEachHexInShape(m_DragStartHex, m_DragEndHex, [this](const Map::HexCoords &h) { ApplyToHex(h); });
        }

        m_MapDragging = false;
        m_LastStampHex.reset();

        if (m_CurrentTool == MapTool::Select) {
            FinalizeSelection();
        } else {
            CommitMapChanges();
        }
    }

    void MapEditState::CancelMapDrag() {
        if (!m_MapDragging) {
            return;
        }

        if (m_CurrentGrid) {
            for (const auto &[hex, pre] : m_PreDragState) {
                if (pre) {
                    m_CurrentGrid->EmplaceTile(hex, pre->first, pre->second);
                } else {
                    m_CurrentGrid->RemoveTileAt(hex);
                }
            }
            if (m_MapComp && !m_PreDragState.empty()) {
                m_MapComp->needsMeshUpdate = true;
            }
        }
        m_PreDragState.clear();
        m_PostDragState.clear();

        if (m_CurrentTool == MapTool::Select) {
            m_Selection = m_SelectionAtDragStart;
            SyncSelection();
        }

        m_MapDragging = false;
        m_LastStampHex.reset();
    }

    void MapEditState::StampBrushAlongStroke(const Map::HexCoords &hex) {
        if (!m_LastStampHex) {
            StampBrush(hex);
        } else if (*m_LastStampHex != hex) {
            for (const auto &step : Math::HexMath::GetHexLine(*m_LastStampHex, hex)) {
                StampBrush(step);
            }
        } else {
            return;
        }
        m_LastStampHex = hex;
    }

    void MapEditState::StampBrush(const Map::HexCoords &center) {
        ForEachHexWithinRadius(center, BrushRings(), [this](const Map::HexCoords &h) { ApplyToHex(h); });
    }

    //
    // Per hex operations
    //

    void MapEditState::ApplyToHex(const Map::HexCoords &hex) {
        switch (m_CurrentTool) {
            case MapTool::Paint:
                PaintHex(hex);
                break;
            case MapTool::Erase:
                EraseHex(hex);
                break;
            case MapTool::Select:
                SelectHex(hex);
                break;
        }
    }

    void MapEditState::RecordPreState(const Map::HexCoords &hex, const Map::Tile *existing) {
        std::optional<TileState> snapshot;
        if (existing) {
            snapshot = TileState{existing->type, existing->walkable};
        }
        m_PreDragState.try_emplace(hex, snapshot);
    }

    void MapEditState::PaintHex(const Map::HexCoords &hex) {
        const uint8_t brushType = m_TileEditorPanel.GetSourceType();
        const bool brushWalkable = m_TileEditorPanel.GetBrushWalkable();

        const Map::Tile *existing = m_CurrentGrid->Get(hex);
        if (existing && existing->type == brushType && existing->walkable == brushWalkable) {
            return;
        }

        RecordPreState(hex, existing);
        m_CurrentGrid->EmplaceTile(hex, brushType, brushWalkable);
        m_PostDragState[hex] = TileState{brushType, brushWalkable};
        m_MapComp->needsMeshUpdate = true;
    }

    void MapEditState::EraseHex(const Map::HexCoords &hex) {
        const Map::Tile *existing = m_CurrentGrid->Get(hex);
        if (!existing) {
            return;
        }

        RecordPreState(hex, existing);
        m_CurrentGrid->RemoveTileAt(hex);
        m_PostDragState[hex] = std::nullopt;
        m_MapComp->needsMeshUpdate = true;
    }

    void MapEditState::SelectHex(const Map::HexCoords &hex) {
        if (m_SelectMode == SelectMode::Subtract) {
            m_Selection.erase(hex);
        } else if (m_CurrentGrid->HasTile(hex)) {
            m_Selection.insert(hex);
        }
    }

    //
    // Undo
    //

    std::string MapEditState::FindMapEntityUUID() const {
        for (const auto id : m_EditorLayer->m_Registry->GetLivingEntities()) {
            if (m_EditorLayer->m_Registry->GetComponent<ECS::Components::MapComponent>(id) == m_MapComp) {
                return m_EditorLayer->m_Registry->GetEntityUUID(id);
            }
        }
        return {};
    }

    void MapEditState::CommitMapChanges() {
        if (!m_CurrentGrid || !m_MapComp) {
            m_PreDragState.clear();
            m_PostDragState.clear();
            return;
        }

        TileStateMap oldState;
        TileStateMap newState;
        for (const auto &[hex, post] : m_PostDragState) {
            const auto pre = m_PreDragState.find(hex);
            if (pre == m_PreDragState.end() || pre->second == post) {
                continue;
            }
            oldState.emplace(hex, pre->second);
            newState.emplace(hex, post);
        }
        m_PreDragState.clear();
        m_PostDragState.clear();

        if (newState.empty()) {
            return;
        }

        m_MapComp->needsMeshUpdate = true;
        m_MapComp->mapDirty = true;
        if (m_EditorLayer->m_Scene) {
            m_EditorLayer->m_Scene->MarkAsChanged();
        }

        std::string mapUUID = FindMapEntityUUID();
        if (mapUUID.empty()) {
            LOG_ERROR(LOG_WHO, "Map entity not found, edit applied but not undoable");
            return;
        }

        m_EditorLayer->m_UndoManager.Execute(std::make_unique<Commands::MapChangeTileCommand>(std::move(oldState), std::move(newState), std::move(mapUUID)), *m_EditorLayer->m_Context);
    }

    //
    // Selection
    //

    void MapEditState::ClearSelection() {
        m_Selection.clear();
        m_PrimaryHex.reset();
        if (m_MapState) {
            m_MapState->hasPathTo = false;
        }
        SyncSelection();
    }

    void MapEditState::FinalizeSelection() {
        if (m_Selection.contains(m_DragStartHex)) {
            m_PrimaryHex = m_DragStartHex;
        }
        SyncSelection();

        if (m_PanelTile) {
            m_TileEditorPanel.CopyBrushFromTile(*m_PanelTile);
        }
    }

    void MapEditState::SyncSelection() {
        if (m_CurrentGrid) {
            std::erase_if(m_Selection, [this](const Map::HexCoords &h) { return !m_CurrentGrid->HasTile(h); });
        } else {
            m_Selection.clear();
        }

        if (m_PrimaryHex && !m_Selection.contains(*m_PrimaryHex)) {
            m_PrimaryHex.reset();
        }
        if (!m_PrimaryHex && !m_Selection.empty()) {
            m_PrimaryHex = *m_Selection.begin();
        }

        Map::Tile *resolved = (m_CurrentGrid && m_PrimaryHex) ? m_CurrentGrid->Get(*m_PrimaryHex) : nullptr;
        if (resolved != m_PanelTile) {
            // only push on change: SetSelectedTile resets the panel's edit state
            m_PanelTile = resolved;
            m_TileEditorPanel.SetSelectedTile(resolved);
        }

        if (m_MapState && m_CurrentTool == MapTool::Select) {
            m_MapState->hasPathTo = m_PrimaryHex.has_value();
            if (m_PrimaryHex) {
                m_MapState->pathTo = *m_PrimaryHex;
            }
        }
    }

    void MapEditState::EraseSelected() {
        if (!m_CurrentGrid || !m_MapComp || m_Selection.empty()) {
            return;
        }

        m_PreDragState.clear();
        m_PostDragState.clear();

        for (const auto &hex : m_Selection) {
            EraseHex(hex);
        }
        ClearSelection();
        CommitMapChanges();
    }

    //
    // Rendering
    //

    void MapEditState::OnRender() {
        m_EditorLayer->DrawEditorUI();
        SyncSelection();
        m_TileEditorPanel.OnImGuiRender();

        auto *renderer = m_EditorLayer->m_Context->renderer;
        renderer->BeginFrame();

        const glm::mat4 &vp = renderer->GetCurrentVP();
        const Math::Frustum::ViewFrustum frustum = Math::Frustum::FromCameraVP(vp, Core::HEX_SIZE * 2.0f);

        ECS::Systems::MapRenderSystem::RenderAll(*m_EditorLayer->m_Registry, *m_EditorLayer->m_Context, frustum);

        if (m_MapComp && m_CurrentGrid && m_MapComp->hexMesh && m_MapComp->outlineMat) {
            const auto submitHex = [&](const Map::HexCoords &hex, const float z) {
                const auto worldPos = Map::HexGrid::GetWorldPos(hex);

                Rendering::Transform transform;
                transform.SetPosition({worldPos.x, worldPos.y, z});
                transform.SetScale({kOverlayScale, kOverlayScale, 1.0f});

                renderer->Submit(m_MapComp->hexMesh, m_MapComp->outlineMat, transform);
            };

            if (m_CurrentTool == MapTool::Select) {
                for (const auto &hex : m_Selection) {
                    submitHex(hex, kSelectionZ);
                }
            }

            // shape preview. Brush always shows (it's the cursor), rect/circle only show tiles they'd actually affect for erase/select.
            if (m_EditorLayer->m_ViewportPanel.IsHovered() || m_MapDragging) {
                const bool tilesOnly = IsDragShape() && m_CurrentTool != MapTool::Paint;
                const auto anchor = m_MapDragging ? m_DragStartHex : m_HoveredHex;
                const auto cursor = m_MapDragging ? m_DragEndHex : m_HoveredHex;

                ForEachHexInShape(anchor, cursor, [&](const Map::HexCoords &hex) {
                    if (tilesOnly && !m_CurrentGrid->HasTile(hex)) {
                        return;
                    }
                    submitHex(hex, kPreviewZ);
                });
            }
        }

        renderer->SetLightmap(nullptr);
    }

    //
    // Toolbar
    //

    void MapEditState::OnDrawModeToolbar() {
        DrawMapMenu();
        ToolbarSeparator();
        DrawToolControls();
    }

    void MapEditState::DrawMapMenu() {
        if (m_MapComp) {
            const std::string &mapPath = m_MapComp->mapFilePath;
            if (mapPath.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%sUnsaved Map", m_MapComp->mapDirty ? "* " : "");
            } else if (m_MapComp->mapDirty) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "* %s", mapPath.c_str());
            } else {
                ImGui::TextDisabled("%s", mapPath.c_str());
            }
            ImGui::SameLine();
        }

        if (ImGui::Button("Map##MapMenuBtn")) {
            ImGui::OpenPopup("MapMenuPopup");
        }

        if (!ImGui::BeginPopup("MapMenuPopup")) {
            return;
        }

        if (ImGui::MenuItem("Create New Map", nullptr, false, m_MapComp != nullptr)) {
            m_EditorLayer->PromptSaveDirtyMap([this] {
                if (!m_CurrentGrid || !m_MapComp) {
                    return;
                }
                CancelMapDrag();
                m_CurrentGrid->Clear();
                m_MapComp->typeMats.clear();
                m_MapComp->mapFilePath.clear();
                m_MapComp->mapDirty = false;
                m_MapComp->needsMeshUpdate = true;
                ResetTransientState();
                m_TileEditorPanel.InitBrushFromFirstType();
                m_EditorLayer->m_UndoManager.Clear();
                SetWindowTitle("Unsaved Map");
            });
        }
        if (ImGui::MenuItem("Save Map", nullptr, false, m_MapComp != nullptr)) {
            SaveMap();
        }
        if (ImGui::MenuItem("Save Map As", nullptr, false, m_MapComp != nullptr)) {
            LOG_INFO(LOG_WHO, "Save as requested");
            const std::string mapsDir = GetMapsDirectory();
            if (const auto path = Platform::FileDialogs::SaveFile(*m_EditorLayer->m_Context, {.filterName = "Map File", .filterExt = "obmap", .defaultPath = mapsDir.c_str(), .defaultName = "map.obmap"})) {
                m_MapComp->mapFilePath = IO::VFS::ToRelative(*path);
                m_EditorLayer->SaveScene();
                m_MapComp->mapDirty = false;
                SetWindowTitle(m_MapComp->mapFilePath);
            }
        }
        if (ImGui::MenuItem("Load Map from file", nullptr, false, m_MapComp != nullptr)) {
            m_EditorLayer->PromptSaveDirtyMap([this] { LoadMapFromDialog(); });
        }
        ImGui::EndPopup();
    }

    void MapEditState::LoadMapFromDialog() {
        if (!m_CurrentGrid || !m_MapComp) {
            return;
        }

        LOG_INFO(LOG_WHO, "Load requested");
        const std::string mapsDir = GetMapsDirectory();
        const auto path = Platform::FileDialogs::OpenFile(*m_EditorLayer->m_Context, {.filterName = "Map File", .filterExt = "obmap", .defaultPath = mapsDir.c_str()});
        if (!path) {
            return;
        }

        CancelMapDrag();
        if (IO::MapIO::Deserialize(*path, *m_CurrentGrid)) {
            m_MapComp->mapFilePath = IO::VFS::ToRelative(*path);
            m_MapComp->needsMeshUpdate = true;
            m_MapComp->mapDirty = false;
            ResetTransientState();
            m_EditorLayer->m_UndoManager.Clear();
            LOG_INFO(LOG_WHO, "Loaded from file: " + *path);
            SetWindowTitle(m_MapComp->mapFilePath);
        } else {
            LOG_ERROR(LOG_WHO, "Failed to load map into editor");
        }
    }

    void MapEditState::DrawToolControls() {
        if (ToggleButton("  Paint  ", m_CurrentTool == MapTool::Paint, "Paint tiles (B)\nDrag to paint continuously.")) {
            SetTool(MapTool::Paint);
        }
        ImGui::SameLine();
        if (ToggleButton("  Erase  ", m_CurrentTool == MapTool::Erase, "Remove tiles (E)\nDrag to erase continuously.")) {
            SetTool(MapTool::Erase);
        }
        ImGui::SameLine();
        if (ToggleButton("  Select  ", m_CurrentTool == MapTool::Select, "Select tiles to inspect them (Q)\nShift+drag adds to the selection, Ctrl+drag removes from it.")) {
            SetTool(MapTool::Select);
        }

        ToolbarSeparator();

        if (ToggleButton(" Brush ", m_ToolShape == MapToolShape::RADIALBRUSH, "Round brush. Size is set with the slider or Ctrl+scroll.")) {
            m_ToolShape = MapToolShape::RADIALBRUSH;
        }
        ImGui::SameLine();
        if (ToggleButton(" Rect ", m_ToolShape == MapToolShape::RECT, "Drag a rectangle. Applied on release, Esc cancels.")) {
            m_ToolShape = MapToolShape::RECT;
        }
        ImGui::SameLine();
        if (ToggleButton(" Circle ", m_ToolShape == MapToolShape::CIRC, "Drag from the centre outwards. Applied on release, Esc cancels.")) {
            m_ToolShape = MapToolShape::CIRC;
        }

        if (m_ToolShape == MapToolShape::RADIALBRUSH) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90.0f);
            ImGui::SliderInt("##BrushSize", &m_BrushSize, 1, kMaxBrushSize, "Size %d");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Brush size: 1 = a single hex, 2 = 7 hexes, 3 = 19 ...\nCtrl+scroll in the viewport also changes it.");
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(%d hex%s)", HexCountForSize(m_BrushSize), HexCountForSize(m_BrushSize) == 1 ? "" : "es");
        }

        if (m_CurrentTool == MapTool::Select) {
            ToolbarSeparator();
            ImGui::Text("%zu selected", m_Selection.size());

            ImGui::SameLine();
            ImGui::BeginDisabled(m_Selection.empty());
            if (ImGui::Button("Clear")) {
                ClearSelection();
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                EraseSelected();
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Erase every selected tile (undoable).");
            }
            ImGui::EndDisabled();
        }
    }

    //
    // Saving
    //

    void MapEditState::SaveMap() {
        if (!m_MapComp) {
            return;
        }

        if (m_MapComp->mapFilePath.empty()) {
            // no file yet
            const std::string mapsDir = GetMapsDirectory();
            if (const auto path = Platform::FileDialogs::SaveFile(*m_EditorLayer->m_Context, {.filterName = "Map File", .filterExt = "obmap", .defaultPath = mapsDir.c_str(), .defaultName = "map.obmap"})) {
                m_MapComp->mapFilePath = IO::VFS::ToRelative(*path);
                m_EditorLayer->SaveScene();
                m_MapComp->mapDirty = false;
                SetWindowTitle(m_MapComp->mapFilePath);
            }
        } else {
            LOG_INFO(LOG_WHO, "Save requested");
            m_EditorLayer->SaveScene();
            m_MapComp->mapDirty = false;
        }
    }

    void MapEditState::OnSaveKey() { SaveMap(); }

    //
    // Tile types
    //

    uint8_t MapEditState::GetOrCreateTypeForMaterial(const std::shared_ptr<Rendering::Texture> &tex, const glm::vec4 &color) const {
        if (!m_MapComp) {
            LOG_ERROR(LOG_WHO, "GetOrCreateTypeForMaterial called without a MapComponent");
            return 0;
        }

        auto &typeMats = m_MapComp->typeMats;

        for (const auto &[id, mat] : typeMats) {
            if (mat->texture == tex && mat->color == color) {
                return id;
            }
        }

        // first unused id
        std::array<bool, 256> used{};
        for (const auto &[id, mat] : typeMats) {
            used[id] = true;
        }
        const auto freeIt = std::ranges::find(used, false);
        if (freeIt == used.end()) {
            LOG_ERROR(LOG_WHO, "All 256 tile type ids are in use, reusing the first one");
            return typeMats.front().first;
        }
        const auto newId = static_cast<uint8_t>(std::distance(used.begin(), freeIt));

        const auto shader = !typeMats.empty() ? typeMats.begin()->second->shader : m_EditorLayer->m_Context->resources->Get<Rendering::Shader>("[Engine] Base");

        typeMats.emplace_back(newId, std::make_shared<Rendering::Material>(Rendering::Material{.shader = shader, .texture = tex, .color = color}));
        m_MapComp->needsMeshUpdate = true;
        if (m_EditorLayer->m_Scene) {
            m_EditorLayer->m_Scene->MarkAsChanged();
        }
        return newId;
    }

} // namespace Editor::States

#pragma pop_macro("LOG_WHO")
