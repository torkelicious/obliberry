#include "RegistryPanel.h"
#include "Platform/Threading/SmallTask.h"
#include "EditorWidgets.h"
#include "EditorWidgetsCombo.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Rendering/Material.h"
#include "Rendering/MeshFactory.h"
#include "Rendering/Renderer.h"
#include "IO/Loaders/PrefabManager.h"
#include "Core/Constants.h"

#include <imgui.h>

void Editor::UI::RegistryPanel::OnImGuiRender() {
    ImGui::Begin("Registry");

    m_IsHovered = ImGui::IsWindowHovered();

    if (m_SceneContext) {
        auto &registry = m_SceneContext->GetRegistry();
        const auto &livingEntities = registry.GetLivingEntities();

        int visibleEntities = 0;
        for (const ECS::EntityID id : livingEntities) {
            if (registry.IsValid(id)) {
                if (ECS::Entity e(id, &registry); !e.HasComponent<ECS::Components::MapComponent>())
                    visibleEntities++;
            }
        }
        ImGui::Text("Entities: %d", visibleEntities);
        ImGui::Separator();

        // entity creation
        if (ImGui::Button("+")) {
            const auto newId = registry.CreateEntity();
            m_SelectedEntity = ECS::Entity(newId, &registry);
            m_SelectedEntity.AddComponent<ECS::Components::TransformComponent>();
            auto mesh = std::make_shared<Rendering::Mesh>(Rendering::MeshFactory::CreateQuad());
            mesh->SetFactoryId("Quad");
            Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([mesh] { mesh->InitGL(); }));
            m_SelectedEntity.AddComponent<ECS::Components::MeshComponent>().mesh = std::move(mesh);
            auto &[material] = m_SelectedEntity.AddComponent<ECS::Components::MaterialComponent>();
            material = std::make_shared<Rendering::Material>();
            MarkSceneChanged(m_EngineContext);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Create empty entity");
        }
        ImGui::SameLine();
        if (m_SelectedEntity) {
            if (ImGui::Button("-")) {
                registry.DestroyEntity(static_cast<ECS::EntityID>(m_SelectedEntity));
                m_SelectedEntity = ECS::Entity{};
                MarkSceneChanged(m_EngineContext);
            }
        }

        // prefabs
        ImGui::Spacing();
        static std::string pendingPrefabPath;
        if (FileCombo("Spawn Prefab", std::string(Core::PREFAB_PATH), std::string(".json"), pendingPrefabPath)) {
            if (!pendingPrefabPath.empty() && m_EngineContext) {
                if (const ECS::EntityID newId = IO::PrefabManager::Instantiate(registry, *m_EngineContext->resources, pendingPrefabPath); newId != 0) {
                    m_SelectedEntity = ECS::Entity(newId, &registry);
                    MarkSceneChanged(m_EngineContext);
                }
                pendingPrefabPath.clear();
            }
        }

        ImGui::Separator();

        // entity list
        ImGui::BeginChild("Entity List", ImVec2(0, 0), true);
        for (const ECS::EntityID id : livingEntities) {
            if (!registry.IsValid(id))
                continue;

            ECS::Entity entity(id, &registry);
            const std::string name = entity.GetName();

            // skip map
            if (entity.HasComponent<ECS::Components::MapComponent>())
                continue;

            std::string label = name;
            if (label.empty()) {
                label = "Entity " + std::to_string(id);
            }

            ImGui::PushID(id);
            if (const bool isSelected = static_cast<bool>(m_SelectedEntity) && m_SelectedEntity == entity; ImGui::Selectable(label.c_str(), isSelected)) {
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
