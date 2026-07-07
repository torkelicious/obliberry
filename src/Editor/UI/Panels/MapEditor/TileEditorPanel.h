

#pragma once
#include "Editor/UI/Panels/EditorPanel.h"
#include "Map/Hex.h"

namespace Editor::UI {

    // todo implement  ...very work in progress..
    struct TilePrefab {};

    class TileEditorPanel : public EditorPanel {
    public:
        void OnImGuiRender() override;

    private:
        Map::Tile *m_CurrentTile = nullptr;
        glm::vec4 m_TileColor;
    };

} // namespace Editor::UI
