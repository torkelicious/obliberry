#include "InspectorPanel.h"
#include "EditorWidgets.h"
#include "ECS/Components/CustomDataComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/ParticleEmitterComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/PrefabSourceComponent.h"
#include "IO/Loaders/PrefabManager.h"
#include <cstring>
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
    m_Widgets.push_back(std::make_unique<ParticleEmitterWidget>());
}

Editor::UI::InspectorPanel::~InspectorPanel() = default;

void Editor::UI::InspectorPanel::OnImGuiRender() {
    ImGui::Begin("Inspector");

    m_IsHovered = ImGui::IsWindowHovered();

    if (m_SceneContext && static_cast<bool>(m_SelectedEntity)) {
        // ReSharper disable once CppDFAConstantConditions
        if (m_SelectedEntity) {
            char nameBuffer[256];
            std::string entityName = m_SelectedEntity.GetName();
            if (entityName.empty()) {
                entityName = "Entity " + std::to_string(static_cast<ECS::EntityID>(m_SelectedEntity));
            }
            strncpy(nameBuffer, entityName.c_str(), sizeof(nameBuffer));
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';

            if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
                if (m_UndoManager && m_EngineContext) {
                    m_UndoManager->Execute(std::make_unique<Commands::SetNameCommand>(static_cast<ECS::EntityID>(m_SelectedEntity), entityName, nameBuffer), *m_EngineContext);
                }
                MarkSceneChanged(m_EngineContext);
            }
            ImGui::Separator();
            ImGui::Spacing();

            if (m_SelectedEntity.HasComponent<ECS::Components::PrefabSourceComponent>()) {
                const auto *psc = m_SelectedEntity.GetComponent<ECS::Components::PrefabSourceComponent>();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
                ImGui::Text("Prefab: %s", psc->prefabPath.c_str());
                ImGui::PopStyleColor();
                if (ImGui::Button("Revert")) {
                    auto &registry = m_SceneContext->GetRegistry();
                    const auto entityId = static_cast<ECS::EntityID>(m_SelectedEntity);
                    registry.DestroyEntity(entityId);
                    const ECS::EntityID newId = IO::PrefabManager::Instantiate(registry, *m_EngineContext->resources, psc->prefabPath);
                    m_SelectedEntity = ECS::Entity(newId, &registry);
                    MarkSceneChanged(m_EngineContext);
                    return; // entity changed, skip rest of draw
                }
                ImGui::SameLine();
                if (ImGui::Button("Break Prefab")) {
                    m_SelectedEntity.RemoveComponent<ECS::Components::PrefabSourceComponent>();
                    MarkSceneChanged(m_EngineContext);
                }
                ImGui::Separator();
            }

            // Component widgets
            for (const auto &widget : m_Widgets) {
                ImGui::PushID(widget->GetName());
                widget->Draw(m_SelectedEntity, m_EngineContext, m_UndoManager);
                ImGui::PopID();
            }

            // Save as Prefab
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::Button("Save as Prefab")) {
                ImGui::OpenPopup("SavePrefabPopup");
            }
            if (ImGui::BeginPopup("SavePrefabPopup")) {
                static char prefabNameBuf[128] = "";
                if (ImGui::IsWindowAppearing()) {
                    const std::string entityName = m_SelectedEntity.GetName();
                    strncpy(prefabNameBuf, entityName.c_str(), sizeof(prefabNameBuf) - 1);
                    prefabNameBuf[sizeof(prefabNameBuf) - 1] = '\0';
                }
                ImGui::InputText("Name", prefabNameBuf, sizeof(prefabNameBuf));
                if (ImGui::Button("Save") && prefabNameBuf[0] != '\0') {
                    const std::string path = "assets/prefabs/" + std::string(prefabNameBuf) + ".json";
                    IO::PrefabManager::SavePrefab(m_SelectedEntity, path, *m_EngineContext->resources);
                    MarkSceneChanged(m_EngineContext);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Components");
            if (ImGui::Button("Add Component")) {
                ImGui::OpenPopup("AddComponentPopup");
            }
            if (ImGui::BeginPopup("AddComponentPopup")) {
                const auto entId = static_cast<ECS::EntityID>(m_SelectedEntity);

                struct CompEntry {
                    const char *name;
                    bool has;
                    std::function<void()> add;
                };
                const CompEntry entries[] = {
                        {.name = "Transform",
                         .has = m_SelectedEntity.HasComponent<ECS::Components::TransformComponent>(),
                         .add = [this,
                                 entId] { m_UndoManager->Execute(std::make_unique<Commands::AddComponentCommand<ECS::Components::TransformComponent>>(entId, ECS::Components::TransformComponent{}), *m_EngineContext); }},
                        {.name = "Point Light",
                         .has = m_SelectedEntity.HasComponent<ECS::Components::PointLightComponent>(),
                         .add =
                                 [this, entId] {
                                     m_UndoManager->Execute(std::make_unique<Commands::AddComponentCommand<ECS::Components::PointLightComponent>>(entId, ECS::Components::PointLightComponent{}), *m_EngineContext);
                                 }},
                        {.name = "Movement",
                         .has = m_SelectedEntity.HasComponent<ECS::Components::MovementComponent>(),
                         .add = [this,
                                 entId] { m_UndoManager->Execute(std::make_unique<Commands::AddComponentCommand<ECS::Components::MovementComponent>>(entId, ECS::Components::MovementComponent{}), *m_EngineContext); }},
                        {.name = "Mesh",
                         .has = m_SelectedEntity.HasComponent<ECS::Components::MeshComponent>(),
                         .add = [this, entId] { m_UndoManager->Execute(std::make_unique<Commands::AddComponentCommand<ECS::Components::MeshComponent>>(entId, ECS::Components::MeshComponent{}), *m_EngineContext); }},
                        {.name = "Material",
                         .has = m_SelectedEntity.HasComponent<ECS::Components::MaterialComponent>(),
                         .add = [this,
                                 entId] { m_UndoManager->Execute(std::make_unique<Commands::AddComponentCommand<ECS::Components::MaterialComponent>>(entId, ECS::Components::MaterialComponent{}), *m_EngineContext); }},
                        {.name = "Directional Texture",
                         .has = m_SelectedEntity.HasComponent<ECS::Components::DirectionalTextureComponent>(),
                         .add =
                                 [this, entId] {
                                     m_UndoManager->Execute(std::make_unique<Commands::AddComponentCommand<ECS::Components::DirectionalTextureComponent>>(entId, ECS::Components::DirectionalTextureComponent{}),
                                                            *m_EngineContext);
                                 }},
                        {.name = "Particle Emitter",
                         .has = m_SelectedEntity.HasComponent<ECS::Components::ParticleEmitterComponent>(),
                         .add =
                                 [this, entId] {
                                     if (!m_SelectedEntity.HasComponent<ECS::Components::TransformComponent>())
                                         m_UndoManager->Execute(std::make_unique<Commands::AddComponentCommand<ECS::Components::TransformComponent>>(entId, ECS::Components::TransformComponent{}), *m_EngineContext);
                                     m_UndoManager->Execute(std::make_unique<Commands::AddComponentCommand<ECS::Components::ParticleEmitterComponent>>(entId, ECS::Components::ParticleEmitterComponent{}),
                                                            *m_EngineContext);
                                 }},
                        {.name = "Scripts", .has = m_SelectedEntity.HasComponent<ECS::Components::ScriptComponent>(), .add = [this] { m_SelectedEntity.AddComponent<ECS::Components::ScriptComponent>(); }},
                        {.name = "ObSL Custom Data",
                         .has = m_SelectedEntity.HasComponent<ECS::Components::CustomDataComponent>(),
                         .add =
                                 [this, entId] {
                                     m_UndoManager->Execute(std::make_unique<Commands::AddComponentCommand<ECS::Components::CustomDataComponent>>(entId, ECS::Components::CustomDataComponent{}), *m_EngineContext);
                                 }},
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
