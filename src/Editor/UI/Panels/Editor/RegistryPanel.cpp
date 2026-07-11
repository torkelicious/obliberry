#include "RegistryPanel.h"

#include "EditorWidgets.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Rendering/Material.h"
#include "Rendering/MeshFactory.h"
#include "Rendering/Renderer.h"

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
            if (const bool isSelected = static_cast<bool>(m_SelectedEntity) && m_SelectedEntity == entity; ImGui::Selectable(label.c_str(), isSelected)) {
                m_SelectedEntity = entity;
            }
            ImGui::PopID();
        }
        ImGui::Separator();
        if (ImGui::Button("+")) {
            const auto newId = registry.CreateEntity();
            m_SelectedEntity = ECS::Entity(newId, &registry);

            m_SelectedEntity.AddComponent<ECS::Components::TransformComponent>();
            auto mesh = std::make_shared<Rendering::Mesh>(Rendering::MeshFactory::CreateQuad());
            mesh->SetFactoryId("Quad");
            Rendering::Renderer::SubmitInitTask([mesh] { mesh->InitGL(); });
            m_SelectedEntity.AddComponent<ECS::Components::MeshComponent>().mesh = std::move(mesh);
            auto &[material] = m_SelectedEntity.AddComponent<ECS::Components::MaterialComponent>();
            material = std::make_shared<Rendering::Material>();
            MarkSceneChanged(m_EngineContext);
        }
        ImGui::SameLine();
        if (m_SelectedEntity) {
            if (ImGui::Button("-")) {
                registry.DestroyEntity(static_cast<ECS::EntityID>(m_SelectedEntity));
                m_SelectedEntity = ECS::Entity{};
                MarkSceneChanged(m_EngineContext);
            }
        }
        ImGui::EndChild();
    } else {
        ImGui::TextDisabled("No scene loaded.");
    }

    ImGui::End();
}
