#pragma once
#include "../EditorState.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "Map/Hex.h"

#include <cstdint>

namespace Editor {

    struct Brush {};

    class MapEditState : public EditorState {
    public:
        enum Tool : uint8_t { Paint, Erase, Select };

        void OnEnter() override;

        void OnUpdate(float dt) override;

        void OnHandleInput(float dt) override;

        void OnDrawPanels() override;

        void OnRender() override;

        void OnDrawModeToolbar() override;
        void OnDrawUtilityWindows() override;

        void OnExit() override;

        void ToolClickEvent(int btn = 0);

        uint8_t GetOrCreateTypeForTexture(const std::shared_ptr<Rendering::Texture> &tex,
                                          const glm::vec4 &color = {1, 1, 1, 1}) const;

    private:
        Map::HexGrid *m_CurrentGrid = nullptr;
        ECS::Components::MapStateComponent *m_MapState = nullptr;
        ECS::Components::MapComponent *m_MapComp = nullptr;
        Tool m_CurrentTool = Paint;

        // makes it easier than using the dumb names in the component
        Map::HexCoords m_hoveredHex;  // what the comp would refer to as "selectedHex", stupid naming.. i know.
        Map::HexCoords m_selectedHex; // actually clicked hex, not the same as the components "selectedHex"
    };
} // namespace Editor
