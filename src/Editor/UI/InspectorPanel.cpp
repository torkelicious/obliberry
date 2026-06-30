#include "InspectorPanel.h"
#include "EditorWidgets.h"
#include <imgui.h>

Editor::UI::InspectorPanel::InspectorPanel() {
    m_Widgets.push_back(std::make_unique<TransformWidget>());
    m_Widgets.push_back(std::make_unique<PointLightWidget>());
    m_Widgets.push_back(std::make_unique<MovementWidget>());
    m_Widgets.push_back(std::make_unique<MeshWidget>());
    m_Widgets.push_back(std::make_unique<MaterialWidget>());
    m_Widgets.push_back(std::make_unique<DirectionalTextureWidget>());
    m_Widgets.push_back(std::make_unique<MapWidget>());
    m_Widgets.push_back(std::make_unique<MapStateWidget>());
    m_Widgets.push_back(std::make_unique<ScriptWidget>());
    m_Widgets.push_back(std::make_unique<CustomDataWidget>());
}

Editor::UI::InspectorPanel::~InspectorPanel() = default;

void Editor::UI::InspectorPanel::OnImGuiRender() {
    ImGui::Begin("Inspector");

    m_IsHovered = ImGui::IsWindowHovered();

    if (m_SceneContext && static_cast<bool>(m_SelectedEntity)) {
        if (m_SelectedEntity) {
            std::string name = m_SelectedEntity.GetName();
            if (name.empty()) {
                name = "Entity " + std::to_string(static_cast<ECS::EntityID>(m_SelectedEntity));
            }
            ImGui::Text("Entity: %s", name.c_str());
            ImGui::Separator();
            ImGui::Spacing();

            for (const auto &widget: m_Widgets) {
                widget->Draw(m_SelectedEntity);
            }
        } else {
            m_SelectedEntity = ECS::Entity{};
            ImGui::TextDisabled("Selected entity no longer exists.");
        }
    } else {
        ImGui::TextDisabled("No entity selected.");
    }

    ImGui::End();
}
