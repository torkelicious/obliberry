#pragma once
#include <ECS/Components/MapComponent.h>
#include <Editor/UI/Panels/EditorPanel.h>
#include <Map/Hex.h>
#include <Rendering/Material.h>
#include <Rendering/Texture.h>

#include <functional>
#include <memory>
#include <unordered_map>

namespace Editor::UI {

    // todo implement  ...very work in progress..
    struct TilePrefab {};

    class TileEditorPanel : public EditorPanel {
    public:
        using CreateTypeFn =
                std::function<uint8_t(const std::shared_ptr<Rendering::Texture> &tex, const glm::vec4 &color)>;

        void OnImGuiRender() override;

        void SetSelectedTile(Map::Tile *tile);
        void SetMapComponent(ECS::Components::MapComponent *mapComp) { m_MapComp = mapComp; }
        void SetCreateTypeFn(CreateTypeFn fn) { m_CreateTypeFn = std::move(fn); }

    private:
        Map::Tile *m_CurrentTile = nullptr;
        ECS::Components::MapComponent *m_MapComp = nullptr;
        CreateTypeFn m_CreateTypeFn;

        std::shared_ptr<Rendering::Texture> m_EditTexture;
        glm::vec4 m_EditColor = {1.0f, 1.0f, 1.0f, 1.0f};
        uint8_t m_EditSourceType = 0;
        bool m_EditInitialized = false;
    };

} // namespace Editor::UI
