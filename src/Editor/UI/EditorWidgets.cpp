#include "EditorWidgets.h"
#include "ECS/ECS.h"
#include <cstring>

Editor::UI::PointLightWidget::PointLightWidget() : AutoComponentWidget("Point Light") {
    m_Fields.push_back({"Color", FieldType::Color3, offsetof(ECS::Components::PointLightComponent, color)});
    m_Fields.push_back({
        "Radius", FieldType::Float, offsetof(ECS::Components::PointLightComponent, radius)
    });
    m_Fields.push_back({
        "Intensity", FieldType::Float, offsetof(ECS::Components::PointLightComponent, intensity)
    });
}

Editor::UI::TransformWidget::TransformWidget() : AutoComponentWidget("Transform") {
}

void Editor::UI::TransformWidget::DrawExtras(ECS::Entity entity, ECS::Components::TransformComponent *component) {
    auto pos = component->transform.GetPosition();
    if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
        component->transform.SetPosition(pos);
    }

    auto rot = component->transform.GetRotation();
    if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f)) {
        component->transform.SetRotation(rot);
    }

    auto scale = component->transform.GetScale();
    if (ImGui::DragFloat3("Scale", &scale.x, 0.1f)) {
        component->transform.SetScale(scale);
    }

    ImGui::Spacing();
    const bool hasBillboard = entity.HasComponent<ECS::Components::BillboardTagComponent>();
    bool useBillboard = hasBillboard;

    if (ImGui::Checkbox("Use Billboard", &useBillboard)) {
        if (useBillboard && !hasBillboard)
            entity.AddComponent<ECS::Components::BillboardTagComponent>();
        else if (!useBillboard && hasBillboard)
            entity.RemoveComponent<ECS::Components::BillboardTagComponent>();
    }
}

Editor::UI::MovementWidget::MovementWidget() : AutoComponentWidget("Movement") {
    m_Fields.push_back({
        "Time Per Step", FieldType::Float, offsetof(ECS::Components::MovementComponent, timePerStep)
    });
    m_Fields.push_back({
        "Step Timer", FieldType::Float, offsetof(ECS::Components::MovementComponent, stepTimer)
    });
    m_Fields.push_back({
        "Idle Timer", FieldType::Float, offsetof(ECS::Components::MovementComponent, idleTimer)
    });
    m_Fields.push_back({
        "Is Moving", FieldType::Bool, offsetof(ECS::Components::MovementComponent, isMoving)
    });
}

void Editor::UI::MovementWidget::DrawExtras(ECS::Entity entity, ECS::Components::MovementComponent *component) {
    ImGui::Text("Path Nodes: %zu", component->currentPath.size());
    ImGui::Text("Current Path Index: %zu", component->currentPathIndex);
}

const char *Editor::UI::MeshWidget::GetName() const { return "Mesh"; }

void Editor::UI::MeshWidget::Draw(const ECS::Entity entity) {
    if (!entity.HasComponent<ECS::Components::MeshComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto *comp = entity.GetComponent<ECS::Components::MeshComponent>();
        ImGui::Text("Mesh Status: %s", comp->mesh ? "Loaded" : "Empty");
    }
}

const char *Editor::UI::MaterialWidget::GetName() const { return "Material"; }

void Editor::UI::MaterialWidget::Draw(const ECS::Entity entity) {
    if (!entity.HasComponent<ECS::Components::MaterialComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto *comp = entity.GetComponent<ECS::Components::MaterialComponent>();
        ImGui::Text("Material Status: %s", comp->material ? "Assigned" : "Empty");
    }
}

const char *Editor::UI::DirectionalTextureWidget::GetName() const { return "Directional Texture"; }

void Editor::UI::DirectionalTextureWidget::Draw(const ECS::Entity entity) {
    if (!entity.HasComponent<ECS::Components::DirectionalTextureComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto *comp = entity.GetComponent<ECS::Components::DirectionalTextureComponent>();
        ImGui::SliderInt("Facing Index", &comp->index, 0, 5);
        for (int i = 0; i < 6; i++) {
            ImGui::BulletText("Direction %d: %s", i, comp->textures[i] ? "Loaded" : "Empty");
        }
    }
}

const char *Editor::UI::MapWidget::GetName() const { return "Map"; }

void Editor::UI::MapWidget::Draw(const ECS::Entity entity) {
    if (!entity.HasComponent<ECS::Components::MapComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto *comp = entity.GetComponent<ECS::Components::MapComponent>();
        char buffer[256];
        strncpy(buffer, comp->mapFilePath.c_str(), sizeof(buffer));
        if (ImGui::InputText("File Path", buffer, sizeof(buffer)))
            comp->mapFilePath = buffer;
        ImGui::Checkbox("Needs Mesh Update", &comp->needsMeshUpdate);
        ImGui::Text("Render Visibles: %zu types", comp->visibles.size());
        ImGui::Text("Hex Mesh: %s", comp->hexMesh ? "Loaded" : "Missing");
    }
}

const char *Editor::UI::MapStateWidget::GetName() const { return "Map State"; }

void Editor::UI::MapStateWidget::Draw(const ECS::Entity entity) {
    if (!entity.HasComponent<ECS::Components::MapStateComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto *comp = entity.GetComponent<ECS::Components::MapStateComponent>();
        ImGui::Checkbox("Has Selection", &comp->hasSelection);
        if (comp->hasSelection)
            ImGui::Text("Selected Hex: [%d, %d]", comp->selectedHex.q, comp->selectedHex.r);
        ImGui::Checkbox("Has Path To", &comp->hasPathTo);
        if (comp->hasPathTo)
            ImGui::Text("Path Target: [%d, %d]", comp->pathTo.q, comp->pathTo.r);
    }
}

const char *Editor::UI::ScriptWidget::GetName() const { return "Scripts"; }

void Editor::UI::ScriptWidget::Draw(const ECS::Entity entity) {
    if (!entity.HasComponent<ECS::Components::ScriptComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto *comp = entity.GetComponent<ECS::Components::ScriptComponent>();
        ImGui::Text("Attached Scripts: %zu", comp->scriptPaths.size());
        ImGui::Separator();
        for (size_t i = 0; i < comp->scriptPaths.size(); i++) {
            ImGui::BulletText("%s", comp->scriptPaths[i].c_str());
            ImGui::Indent();
            ImGui::TextDisabled("Initialized: %s", comp->isInitialized[i] ? "Yes" : "No");
            ImGui::Unindent();
        }
    }
}

const char *Editor::UI::CustomDataWidget::GetName() const { return "ObSL Custom Data"; }

void Editor::UI::CustomDataWidget::Draw(const ECS::Entity entity) {
    if (!entity.HasComponent<ECS::Components::CustomDataComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto *comp = entity.GetComponent<ECS::Components::CustomDataComponent>();
        if (comp->script_components.empty()) {
            ImGui::TextDisabled("No script variables defined.");
            return;
        }
        for (const auto &varName: comp->script_components | std::views::keys) {
            ImGui::BulletText("%s", varName.c_str());
        }
    }
}
