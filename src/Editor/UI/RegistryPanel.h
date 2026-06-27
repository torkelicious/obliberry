#pragma once

#include "ECS/Entity.h"
#include "EditorPanel.h"

class RegistryPanel : public EditorPanel {
public:
    void OnImGuiRender() override;

    Entity GetSelectedEntity() const { return m_SelectedEntity; }
    void SetSelectedEntity(const Entity entity) { m_SelectedEntity = entity; }

private:
    Entity m_SelectedEntity;
};
