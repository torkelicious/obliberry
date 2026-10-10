#pragma once

#include "ECS/Entity.h"
#include "Applications/Editor/UI/Panels/EditorPanel.h"
#include <memory>
#include <vector>
#include <string>

namespace Editor::UI {
    struct IComponentWidget;

    class InspectorPanel : public EditorPanel {
    public:
        InspectorPanel();

        ~InspectorPanel() override;

        void OnImGuiRender() override;

        void SetSelectedEntity(const ECS::Entity entity) { m_SelectedEntity = entity; }
        void Reset() { m_SelectedEntity = ECS::Entity{}; }

    private:
        ECS::Entity m_SelectedEntity;
        std::vector<std::unique_ptr<IComponentWidget>> m_Widgets;
        std::string m_NameBeforeEdit;
    };
} // namespace Editor::UI
