#include "RegistryPanel.h"
#include "EditorWidgets.h"
#include "EditorWidgetsCombo.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RelationshipComponent.h"
#include "Rendering/Material.h"
#include "Rendering/Mesh.h"
#include "IO/Loaders/PrefabManager.h"
#include "Core/Constants.h"
#include "Core/ResourceManager.h"

#include <imgui.h>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace {
    bool IsAncestorOf(ECS::Registry &registry, ECS::EntityID entity, ECS::EntityID candidate) {
        auto *rel = registry.GetComponent<ECS::Components::RelationshipComponent>(entity);
        while (rel && rel->parent != 0) {
            if (rel->parent == candidate)
                return true;
            rel = registry.GetComponent<ECS::Components::RelationshipComponent>(rel->parent);
        }
        return false;
    }

    std::string GetLabel(const ECS::Entity &entity, ECS::EntityID id) {
        std::string label = entity.GetName();
        if (label.empty())
            label = "Entity " + std::to_string(id);
        return label;
    }
} // namespace

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

            auto &resources = *m_EngineContext->resources;
            m_SelectedEntity.AddComponent<ECS::Components::MeshComponent>().mesh = resources.Get<Rendering::Mesh>("[Engine] Quad");
            m_SelectedEntity.AddComponent<ECS::Components::MaterialComponent>().material = resources.Get<Rendering::Material>("[Engine] DefaultMaterial");

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

        // build hierarchy
        std::unordered_set<ECS::EntityID> visibleSet;
        for (const ECS::EntityID id : livingEntities) {
            if (registry.IsValid(id)) {
                ECS::Entity e(id, &registry);
                if (!e.HasComponent<ECS::Components::MapComponent>())
                    visibleSet.insert(id);
            }
        }

        std::unordered_map<ECS::EntityID, std::vector<ECS::EntityID>> parentToChildren;
        std::unordered_set<ECS::EntityID> hasVisibleParent;

        for (const ECS::EntityID id : visibleSet) {
            auto *rel = registry.GetComponent<ECS::Components::RelationshipComponent>(id);
            if (rel && rel->parent != 0 && visibleSet.contains(rel->parent)) {
                parentToChildren[rel->parent].push_back(id);
                hasVisibleParent.insert(id);
            }
        }

        // tree
        ImGui::BeginChild("Entity List", ImVec2(0, 0), true);

        static bool s_Dragging = false;

        std::function<void(ECS::EntityID)> renderNode = [&](ECS::EntityID id) {
            if (!registry.IsValid(id))
                return;

            ECS::Entity entity(id, &registry);
            const std::string label = GetLabel(entity, id);

            const auto *rel = registry.GetComponent<ECS::Components::RelationshipComponent>(id);
            const bool hasChildren = rel && !rel->children.empty();
            const bool isSelected = static_cast<bool>(m_SelectedEntity) && m_SelectedEntity == entity;

            ImGui::PushID(static_cast<int>(id));

            // node
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (!hasChildren)
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (isSelected)
                flags |= ImGuiTreeNodeFlags_Selected;

            const bool nodeOpen = ImGui::TreeNodeEx("##node", flags, "%s", label.c_str());

            // selection
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                m_SelectedEntity = entity;

            // drag
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                s_Dragging = true;
                ImGui::SetDragDropPayload("ENTITY_DRAG", &id, sizeof(ECS::EntityID));
                ImGui::TextUnformatted(label.c_str());
                ImGui::EndDragDropSource();
            } else if (s_Dragging && ImGui::GetDragDropPayload() == nullptr) {
                s_Dragging = false;
            }

            // drop
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG")) {
                    const ECS::EntityID draggedId = *static_cast<const ECS::EntityID *>(payload->Data);
                    // prevent self parenting and ancestor cycles
                    if (draggedId != id && !IsAncestorOf(registry, id, draggedId)) {
                        registry.Reparent(draggedId, id);
                        MarkSceneChanged(m_EngineContext);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // highlight
            if (s_Dragging && ImGui::IsItemHovered()) {
                const ImU32 col = ImGui::GetColorU32(ImGuiCol_NavHighlight);
                ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), col, ImGui::GetStyle().FrameRounding);
            }

            // context menu
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Create Child")) {
                    const auto childId = registry.CreateEntity();
                    registry.Reparent(childId, id);
                    registry.SetEntityName(childId, "Child of " + label);
                    registry.AddComponent<ECS::Components::TransformComponent>(childId);
                    auto &resources = *m_EngineContext->resources;
                    registry.AddComponent<ECS::Components::MeshComponent>(childId).mesh = resources.Get<Rendering::Mesh>("[Engine] Quad");
                    registry.AddComponent<ECS::Components::MaterialComponent>(childId).material = resources.Get<Rendering::Material>("[Engine] DefaultMaterial");
                    m_SelectedEntity = ECS::Entity(childId, &registry);
                    MarkSceneChanged(m_EngineContext);
                }

                if (rel && rel->parent != 0) {
                    if (ImGui::MenuItem("Detach")) {
                        registry.Reparent(id, 0);
                        MarkSceneChanged(m_EngineContext);
                    }
                }

                ImGui::Separator();

                if (ImGui::BeginMenu("Set Parent")) {
                    for (const ECS::EntityID otherId : visibleSet) {
                        if (otherId == id)
                            continue;
                        if (IsAncestorOf(registry, otherId, id))
                            continue;
                        ECS::Entity other(otherId, &registry);
                        if (ImGui::MenuItem(GetLabel(other, otherId).c_str())) {
                            registry.Reparent(id, otherId);
                            MarkSceneChanged(m_EngineContext);
                        }
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndPopup();
            }

            // children
            if (nodeOpen) {
                if (hasChildren) {
                    for (const ECS::EntityID childId : rel->children) {
                        if (visibleSet.contains(childId))
                            renderNode(childId);
                    }
                    ImGui::TreePop();
                }
            }

            ImGui::PopID();
        };

        // render roots
        for (const ECS::EntityID id : livingEntities) {
            if (!registry.IsValid(id))
                continue;
            if (!visibleSet.contains(id))
                continue;
            if (hasVisibleParent.contains(id))
                continue;
            renderNode(id);
        }

        ImGui::EndChild();
    } else {
        ImGui::TextDisabled("No scene loaded.");
    }
    ImGui::End();
}
