#include "InspectorPanel.h"
#include "EditorWidgets.h"
#include "ECS/Components/CustomDataComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include <functional>
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
        // ReSharper disable once CppDFAConstantConditions
        if (m_SelectedEntity) {
            std::string name = m_SelectedEntity.GetName();
            if (name.empty()) {
                name = "Entity " + std::to_string(static_cast<ECS::EntityID>(m_SelectedEntity));
            }
            ImGui::Text("Entity: %s", name.c_str());
            ImGui::Separator();
            ImGui::Spacing();

            for (const auto &widget : m_Widgets) {
                widget->Draw(m_SelectedEntity, m_EngineContext);
            }
            ImGui::Separator();

            if (ImGui::Button("Add Component")) {
                ImGui::OpenPopup("AddComponentPopup");
            }
            if (ImGui::BeginPopup("AddComponentPopup")) {
                struct CompEntry {
                    const char *name;
                    bool has;
                    std::function<void()> add;
                };
                const CompEntry entries[] = {
                        {"Transform", m_SelectedEntity.HasComponent<ECS::Components::TransformComponent>(),
                         [this] { m_SelectedEntity.AddComponent<ECS::Components::TransformComponent>(); }},
                        {"Point Light", m_SelectedEntity.HasComponent<ECS::Components::PointLightComponent>(),
                         [this] { m_SelectedEntity.AddComponent<ECS::Components::PointLightComponent>(); }},
                        {"Movement", m_SelectedEntity.HasComponent<ECS::Components::MovementComponent>(),
                         [this] { m_SelectedEntity.AddComponent<ECS::Components::MovementComponent>(); }},
                        {"Mesh", m_SelectedEntity.HasComponent<ECS::Components::MeshComponent>(),
                         [this] { m_SelectedEntity.AddComponent<ECS::Components::MeshComponent>(); }},
                        {"Material", m_SelectedEntity.HasComponent<ECS::Components::MaterialComponent>(),
                         [this] { m_SelectedEntity.AddComponent<ECS::Components::MaterialComponent>(); }},
                        {"Directional Texture",
                         m_SelectedEntity.HasComponent<ECS::Components::DirectionalTextureComponent>(),
                         [this] { m_SelectedEntity.AddComponent<ECS::Components::DirectionalTextureComponent>(); }},
                        {"Map", m_SelectedEntity.HasComponent<ECS::Components::MapComponent>(),
                         [this] { m_SelectedEntity.AddComponent<ECS::Components::MapComponent>(); }},
                        {"Map State", m_SelectedEntity.HasComponent<ECS::Components::MapStateComponent>(),
                         [this] { m_SelectedEntity.AddComponent<ECS::Components::MapStateComponent>(); }},
                        {"Scripts", m_SelectedEntity.HasComponent<ECS::Components::ScriptComponent>(),
                         [this] { m_SelectedEntity.AddComponent<ECS::Components::ScriptComponent>(); }},
                        {"ObSL Custom Data", m_SelectedEntity.HasComponent<ECS::Components::CustomDataComponent>(),
                         [this] { m_SelectedEntity.AddComponent<ECS::Components::CustomDataComponent>(); }},
                };

                ImGui::TextDisabled("Available Components");
                ImGui::Separator();
                for (const auto &[name, has, add] : entries) {
                    ImGui::PushID(name);
                    if (has) {
                        ImGui::BeginDisabled();
                        ImGui::Selectable(name);
                        ImGui::EndDisabled();
                    } else if (ImGui::Selectable(name)) {
                        add();
                        MarkSceneChanged(m_EngineContext);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PopID();
                }
                ImGui::EndPopup();
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
