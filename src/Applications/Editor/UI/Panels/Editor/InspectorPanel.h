#pragma once

#include "ECS/Entity.h"
#include "Applications/Editor/UI/Panels/EditorPanel.h"
#include <memory>
#include <vector>

namespace Editor::UI {
    struct IComponentWidget;

    class InspectorPanel : public EditorPanel {
    public:
        InspectorPanel();

        ~InspectorPanel() override;

        public:
        void OnImGuiRender() override;

        void SetSelectedEntity(const ECS::Entity entity) { m_SelectedEntity = entity; }
        void Reset() { m_SelectedEntity = ECS::Entity{}; }

    private:
        ECS::Entity m_SelectedEntity;
        std::vector<std::unique_ptr<IComponentWidget>> m_Widgets;
    };
} // namespace Editor::UI
