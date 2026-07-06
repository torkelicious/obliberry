#include "RegistryPanel.h"
#include <imgui.h>

void Editor::UI::RegistryPanel::OnImGuiRender() {
    ImGui::Begin("Registry");

    m_IsHovered = ImGui::IsWindowHovered();

    if (m_SceneContext) {
        auto &registry = m_SceneContext->GetRegistry();
        const auto &livingEntities = registry.GetLivingEntities();

        ImGui::Text("Entities: %zu", livingEntities.size());
        ImGui::Separator();

        ImGui::BeginChild("Entity List", ImVec2(0, 0), true);
        for (const ECS::EntityID id : livingEntities) {
            if (!registry.IsValid(id))
                continue;

            ECS::Entity entity(id, &registry);
            std::string label = entity.GetName();
            if (label.empty()) {
                label = "Entity " + std::to_string(id);
            }

            ImGui::PushID(id);
            if (const bool isSelected = static_cast<bool>(m_SelectedEntity) && m_SelectedEntity == entity;
                ImGui::Selectable(label.c_str(), isSelected)) {
                m_SelectedEntity = entity;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
    } else {
        ImGui::TextDisabled("No scene loaded.");
    }

    ImGui::End();
}
