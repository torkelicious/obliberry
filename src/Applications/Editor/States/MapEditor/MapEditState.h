#pragma once
#include "Applications/Editor/States/EditorStateBase.h"
#include "Applications/Editor/States/MapEditor/MapTools.h"
#include "Core/UUID.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "Applications/Editor/UI/Panels/MapEditor/TileEditorPanel.h"
#include "Map/Hex.h"
#include "Map/HexCoords.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Editor::States {
    class MapEditState : public EditorStateBase {
    public:
        void OnEnter() override;

        void OnUpdate(float dt) override;

        void OnHandleInput(float dt) override;

        void OnDrawPanels() override;

        void OnRender() override;

        void OnDrawModeToolbar() override;
        void OnDrawUtilityWindows() override;

        void OnExit() override;

        void OnSceneLoaded() override;
        void SaveMap();
        void OnSaveKey() override;

        [[nodiscard]] uint8_t GetOrCreateTypeForMaterial(const std::shared_ptr<Rendering::Texture> &tex, const glm::vec4 &color = {1, 1, 1, 1}) const;

    private:
        using HexCallback = std::function<void(const Map::HexCoords &)>;
        using TileState = std::pair<uint8_t, bool>;
        using TileStateMap = std::unordered_map<Map::HexCoords, std::optional<TileState>, Map::HexCoordsHash>;
        using HexSet = std::unordered_set<Map::HexCoords, Map::HexCoordsHash>;

        enum class SelectMode : uint8_t { Replace, Add, Subtract };

        static constexpr int kMaxBrushSize = 8;

        static void ForEachHexWithinRadius(Map::HexCoords center, int radius, const HexCallback &callback);
        static void ForEachHexInRect(const Map::HexCoords &start, const Map::HexCoords &end, const HexCallback &callback);
        static void ForEachHexInCircle(const Map::HexCoords &center, const Map::HexCoords &edge, const HexCallback &callback);

        void ForEachHexInShape(const Map::HexCoords &anchor, const Map::HexCoords &cursor, const HexCallback &callback) const;

        [[nodiscard]] bool IsDragShape() const { return m_ToolShape == MapToolShape::RECT || m_ToolShape == MapToolShape::CIRC; }
        [[nodiscard]] int BrushRings() const { return m_BrushSize - 1; }

        void SetTool(MapTool tool);
        void BeginMapDrag(const Map::HexCoords &hex);
        void UpdateMapDrag(const Map::HexCoords &hex);
        void FinishMapDrag();
        void CancelMapDrag();

        void StampBrushAlongStroke(const Map::HexCoords &hex);
        void StampBrush(const Map::HexCoords &center);

        void ApplyToHex(const Map::HexCoords &hex);
        void PaintHex(const Map::HexCoords &hex);
        void EraseHex(const Map::HexCoords &hex);
        void SelectHex(const Map::HexCoords &hex);

        void RecordPreState(const Map::HexCoords &hex, const Map::Tile *existing);
        void CommitMapChanges();
        [[nodiscard]] Core::UUID FindMapEntityUUID() const;

        void ClearSelection();
        void FinalizeSelection();
        void SyncSelection();
        void EraseSelected();
        void ApplyToSelection(std::optional<uint8_t> type, std::optional<bool> walkable);

        void HandleCameraInput();
        void ResetTransientState();
        void ApplyOverlayColors() const;
        void LoadMapFromDialog();
        void DrawMapMenu();
        void DrawToolControls();

        Map::HexGrid *m_CurrentGrid = nullptr;
        ECS::Components::MapStateComponent *m_MapState = nullptr;
        ECS::Components::MapComponent *m_MapComp = nullptr;

        std::shared_ptr<Rendering::Material> m_PaintMat;
        std::shared_ptr<Rendering::Material> m_EraseMat;
        std::shared_ptr<Rendering::Material> m_SelectedPreviewMat;
        std::shared_ptr<Rendering::Material> m_SelectedMat;
        void CreateOverlayMaterials();

        MapTool m_CurrentTool = MapTool::Paint;
        MapToolShape m_ToolShape = MapToolShape::RADIALBRUSH;
        int m_BrushSize = 1;

        Map::HexCoords m_HoveredHex = {0, 0};

        HexSet m_Selection;
        HexSet m_SelectionAtDragStart;
        std::optional<Map::HexCoords> m_PrimaryHex;
        SelectMode m_SelectMode = SelectMode::Replace;

        // stroke
        bool m_MapDragging = false;
        bool m_WasMouseDown = false;
        Map::HexCoords m_DragStartHex{};
        Map::HexCoords m_DragEndHex{};
        std::optional<Map::HexCoords> m_LastStampHex;

        TileStateMap m_PreDragState;
        TileStateMap m_PostDragState;

        // ui
        UI::TileEditorPanel m_TileEditorPanel;
    };
} // namespace Editor::States
