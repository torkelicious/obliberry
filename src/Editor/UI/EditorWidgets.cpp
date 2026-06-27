#include "EditorWidgets.h"
#include "ECS/ECS.h"
#include <cstring>

PointLightWidget::PointLightWidget() : AutoComponentWidget("Point Light") {
    m_Fields.push_back({"Color", FieldType::Color3, offsetof(PointLightComponent, color)});
    m_Fields.push_back({"Radius", FieldType::Float, offsetof(PointLightComponent, radius)});
    m_Fields.push_back({"Intensity", FieldType::Float, offsetof(PointLightComponent, intensity)});
}

TransformWidget::TransformWidget() : AutoComponentWidget("Transform") {
}

void TransformWidget::DrawExtras(Entity entity, TransformComponent *component) {
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
    const bool hasBillboard = entity.HasComponent<BillboardTagComponent>();
    bool useBillboard = hasBillboard;

    if (ImGui::Checkbox("Use Billboard", &useBillboard)) {
        if (useBillboard && !hasBillboard)
            entity.AddComponent<BillboardTagComponent>();
        else if (!useBillboard && hasBillboard)
            entity.RemoveComponent<BillboardTagComponent>();
    }
}

MovementWidget::MovementWidget() : AutoComponentWidget("Movement") {
    m_Fields.push_back({"Time Per Step", FieldType::Float, offsetof(MovementComponent, timePerStep)});
    m_Fields.push_back({"Step Timer", FieldType::Float, offsetof(MovementComponent, stepTimer)});
    m_Fields.push_back({"Idle Timer", FieldType::Float, offsetof(MovementComponent, idleTimer)});
    m_Fields.push_back({"Is Moving", FieldType::Bool, offsetof(MovementComponent, isMoving)});
}

void MovementWidget::DrawExtras(Entity entity, MovementComponent *component) {
    ImGui::Text("Path Nodes: %zu", component->currentPath.size());
    ImGui::Text("Current Path Index: %zu", component->currentPathIndex);
}

const char *MeshWidget::GetName() const { return "Mesh"; }

void MeshWidget::Draw(const Entity entity) {
    if (!entity.HasComponent<MeshComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto *comp = entity.GetComponent<MeshComponent>();
        ImGui::Text("Mesh Status: %s", comp->mesh ? "Loaded" : "Empty");
    }
}

const char *MaterialWidget::GetName() const { return "Material"; }

void MaterialWidget::Draw(const Entity entity) {
    if (!entity.HasComponent<MaterialComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto *comp = entity.GetComponent<MaterialComponent>();
        ImGui::Text("Material Status: %s", comp->material ? "Assigned" : "Empty");
    }
}

const char *DirectionalTextureWidget::GetName() const { return "Directional Texture"; }

void DirectionalTextureWidget::Draw(const Entity entity) {
    if (!entity.HasComponent<DirectionalTextureComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto *comp = entity.GetComponent<DirectionalTextureComponent>();
        ImGui::SliderInt("Facing Index", &comp->index, 0, 5);
        for (int i = 0; i < 6; i++) {
            ImGui::BulletText("Direction %d: %s", i, comp->textures[i] ? "Loaded" : "Empty");
        }
    }
}

const char *MapWidget::GetName() const { return "Map"; }

void MapWidget::Draw(const Entity entity) {
    if (!entity.HasComponent<MapComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto *comp = entity.GetComponent<MapComponent>();
        char buffer[256];
        strncpy(buffer, comp->mapFilePath.c_str(), sizeof(buffer));
        if (ImGui::InputText("File Path", buffer, sizeof(buffer)))
            comp->mapFilePath = buffer;
        ImGui::Checkbox("Needs Mesh Update", &comp->needsMeshUpdate);
        ImGui::Text("Render Visibles: %zu types", comp->visibles.size());
        ImGui::Text("Hex Mesh: %s", comp->hexMesh ? "Loaded" : "Missing");
    }
}

const char *MapStateWidget::GetName() const { return "Map State"; }

void MapStateWidget::Draw(const Entity entity) {
    if (!entity.HasComponent<MapStateComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto *comp = entity.GetComponent<MapStateComponent>();
        ImGui::Checkbox("Has Selection", &comp->hasSelection);
        if (comp->hasSelection)
            ImGui::Text("Selected Hex: [%d, %d]", comp->selectedHex.q, comp->selectedHex.r);
        ImGui::Checkbox("Has Path To", &comp->hasPathTo);
        if (comp->hasPathTo)
            ImGui::Text("Path Target: [%d, %d]", comp->pathTo.q, comp->pathTo.r);
    }
}

const char *ScriptWidget::GetName() const { return "Scripts"; }

void ScriptWidget::Draw(const Entity entity) {
    if (!entity.HasComponent<ScriptComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto *comp = entity.GetComponent<ScriptComponent>();
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

const char *CustomDataWidget::GetName() const { return "ObSL Custom Data"; }

void CustomDataWidget::Draw(const Entity entity) {
    if (!entity.HasComponent<CustomDataComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto *comp = entity.GetComponent<CustomDataComponent>();
        if (comp->script_components.empty()) {
            ImGui::TextDisabled("No script variables defined.");
            return;
        }
        for (const auto &varName: comp->script_components | std::views::keys) {
            ImGui::BulletText("%s", varName.c_str());
        }
    }
}
