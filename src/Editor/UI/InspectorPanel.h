#pragma once

#include "ECS/Entity.h"
#include "EditorPanel.h"
#include <memory>
#include <vector>

struct IComponentWidget;

class InspectorPanel : public EditorPanel {
public:
    InspectorPanel();
    ~InspectorPanel();

    void OnImGuiRender() override;

    void SetSelectedEntity(const Entity entity) { m_SelectedEntity = entity; }

private:
    Entity m_SelectedEntity;
    std::vector<std::unique_ptr<IComponentWidget>> m_Widgets;
};
