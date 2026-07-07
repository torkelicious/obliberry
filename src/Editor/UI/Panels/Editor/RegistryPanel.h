#pragma once

#include "ECS/Entity.h"
#include "../EditorPanel.h"

namespace Editor::UI {
    class RegistryPanel : public EditorPanel {
    public:
        void OnImGuiRender() override;

        [[nodiscard]] ECS::Entity GetSelectedEntity() const { return m_SelectedEntity; }
        void SetSelectedEntity(const ECS::Entity entity) { m_SelectedEntity = entity; }

    private:
        ECS::Entity m_SelectedEntity;
    };
} // namespace Editor::UI
