#pragma once

#include "ECS/Entity.h"
#include "Applications/Editor/UI/Panels/EditorPanel.h"

namespace Editor::UI {
    class RegistryPanel : public EditorPanel {
    public:
        void OnImGuiRender() override;

        void SetSelectedEntity(const ECS::Entity entity) { m_SelectedEntity = entity; }
        [[nodiscard]] ECS::Entity GetSelectedEntity() const { return m_SelectedEntity; }
        void Reset() { m_SelectedEntity = ECS::Entity{}; }

    private:
        ECS::Entity m_SelectedEntity;
    };
} // namespace Editor::UI
