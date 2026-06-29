#pragma once

#include "ECS/Entity.h"
#include "EditorPanel.h"
#include <memory>
#include <vector>

namespace Editor::UI {
    struct IComponentWidget;

    class InspectorPanel : public EditorPanel {
    public:
        InspectorPanel();

        ~InspectorPanel() override;

        void OnImGuiRender() override;

        void SetSelectedEntity(const ECS::Entity entity) { m_SelectedEntity = entity; }

    private:
        ECS::Entity m_SelectedEntity;
        std::vector<std::unique_ptr<IComponentWidget> > m_Widgets;
    };
} // namespace Editor::UI
