#pragma once
#include <ECS/Components/MapComponent.h>
#include "Applications/Editor/UI/Panels/EditorPanel.h"
#include "Applications/Editor/States/MapEditor/MapTools.h"
#include <Map/Hex.h>
#include <Map/HexCoords.h>
#include "Rendering/Types/Texture/Texture.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_set>

namespace Editor::UI {

    struct TilePrefab {};

    class TileEditorPanel : public EditorPanel {
    public:
        using HexSet = std::unordered_set<Map::HexCoords, Map::HexCoordsHash>;
        using CreateTypeFn = std::function<uint8_t(const std::shared_ptr<Rendering::Texture> &tex, const glm::vec4 &color)>;
        using ApplyToSelectionFn = std::function<void(std::optional<uint8_t> type, std::optional<bool> walkable)>;

        void OnImGuiRender() override;

        void SetSelection(const HexSet *selection) { m_Selection = selection; }
        void SetMapComponent(ECS::Components::MapComponent *mapComp) { m_MapComp = mapComp; }
        void SetCreateTypeFn(CreateTypeFn fn) { m_CreateTypeFn = std::move(fn); }
        void SetApplyToSelectionFn(ApplyToSelectionFn fn) { m_ApplyToSelectionFn = std::move(fn); }

        [[nodiscard]] uint8_t GetSourceType() const { return m_BrushType; }
        void SetBrushType(uint8_t type);
        void InitBrushFromFirstType();
        void CopyBrushFromTile(const Map::Tile &tile);

        [[nodiscard]] const std::shared_ptr<Rendering::Texture> &GetBrushTexture() const { return m_BrushTexture; }
        [[nodiscard]] const glm::vec4 &GetBrushColor() const { return m_BrushColor; }
        [[nodiscard]] bool GetBrushWalkable() const { return m_BrushWalkable; }

        void SetMapTool(const MapTool &tool) { m_CurrentTool = tool; }
        [[nodiscard]] MapTool GetMapTool() const { return m_CurrentTool; }

    private:
        struct SelectionSummary {
            size_t count = 0;
            size_t distinctTypes = 0;
            std::optional<uint8_t> commonType;
            std::optional<bool> commonWalkable;
        };

        [[nodiscard]] SelectionSummary SummarizeSelection() const;

        const HexSet *m_Selection = nullptr;
        ECS::Components::MapComponent *m_MapComp = nullptr;
        CreateTypeFn m_CreateTypeFn;
        ApplyToSelectionFn m_ApplyToSelectionFn;

        // brush state
        uint8_t m_BrushType = 0;
        std::shared_ptr<Rendering::Texture> m_BrushTexture;
        glm::vec4 m_BrushColor = {1.0f, 1.0f, 1.0f, 1.0f};
        bool m_BrushWalkable = true;
        bool m_BrushInitialized = false;

        // selected-tile
        std::shared_ptr<Rendering::Texture> m_EditTexture;
        glm::vec4 m_EditColor = {1.0f, 1.0f, 1.0f, 1.0f};
        std::optional<uint8_t> m_EditSourceType;
        bool m_EditInitialized = false;

        MapTool m_CurrentTool = MapTool::Paint;
    };

} // namespace Editor::UI
