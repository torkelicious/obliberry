#include "EditorWidgets.h"
#include <cstring>

#include "EditorWidgetsCombo.h"
#include "Core/Constants.h"
#include "Core/EngineContext.h"
#include "Core/ResourceManager.h"
#include "ECS/Components/BillboardTagComponent.h"
#include "ECS/Components/CustomDataComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/ScriptComponent.h"
#include "IO/VFS.h"
#include "Rendering/Renderer.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include <filesystem>


//  PointLightWidget

Editor::UI::PointLightWidget::PointLightWidget() : AutoComponentWidget("Point Light") {
    m_Fields.push_back({"Color", FieldType::Color3, offsetof(ECS::Components::PointLightComponent, color)});
    m_Fields.push_back({"Radius", FieldType::Float, offsetof(ECS::Components::PointLightComponent, radius)});
    m_Fields.push_back({"Intensity", FieldType::Float, offsetof(ECS::Components::PointLightComponent, intensity)});
}

//  TransformWidget

Editor::UI::TransformWidget::TransformWidget() : AutoComponentWidget("Transform") {}

void Editor::UI::TransformWidget::DrawExtras(ECS::Entity entity, ECS::Components::TransformComponent *component,
                                             Core::EngineContext *engineContext) {
    auto pos = component->transform.GetPosition();
    if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
        component->transform.SetPosition(pos);
    }
    if (ImGui::IsItemDeactivatedAfterEdit())
        MarkSceneChanged(engineContext);

    const bool isBillboard = entity.HasComponent<ECS::Components::BillboardTagComponent>();

    if (isBillboard) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " Rotation has no effect on Billboard Sprites");
        ImGui::BeginDisabled();
    }

    auto rot = component->transform.GetRotation();
    if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f)) {
        component->transform.SetRotation(rot);
    }

    if (isBillboard) {
        ImGui::EndDisabled();
    }


    if (ImGui::IsItemDeactivatedAfterEdit())
        MarkSceneChanged(engineContext);

    auto scale = component->transform.GetScale();
    if (ImGui::DragFloat3("Scale", &scale.x, 0.1f)) {
        component->transform.SetScale(scale);
    }
    if (ImGui::IsItemDeactivatedAfterEdit())
        MarkSceneChanged(engineContext);

    ImGui::Spacing();
    const bool hasBillboard = entity.HasComponent<ECS::Components::BillboardTagComponent>();
    bool useBillboard = hasBillboard;

    if (ImGui::Checkbox("Use Billboard", &useBillboard)) {
        if (useBillboard && !hasBillboard)
            entity.AddComponent<ECS::Components::BillboardTagComponent>();
        else if (!useBillboard && hasBillboard)
            entity.RemoveComponent<ECS::Components::BillboardTagComponent>();
        MarkSceneChanged(engineContext);
    }
}

//  MovementWidget

Editor::UI::MovementWidget::MovementWidget() : AutoComponentWidget("Movement") {
    m_Fields.push_back({"Time Per Step", FieldType::Float, offsetof(ECS::Components::MovementComponent, timePerStep)});
    m_Fields.push_back({"Step Timer", FieldType::Float, offsetof(ECS::Components::MovementComponent, stepTimer)});
    m_Fields.push_back({"Idle Timer", FieldType::Float, offsetof(ECS::Components::MovementComponent, idleTimer)});
    m_Fields.push_back({"Is Moving", FieldType::Bool, offsetof(ECS::Components::MovementComponent, isMoving)});
}

void Editor::UI::MovementWidget::DrawExtras(ECS::Entity entity, ECS::Components::MovementComponent *component,
                                            Core::EngineContext *engineContext) {
    ImGui::Text("Path Nodes: %zu", component->currentPath.size());
    ImGui::Text("Current Path Index: %zu", component->currentPathIndex);
}

//  MeshWidget

const char *Editor::UI::MeshWidget::GetName() const { return "Mesh"; }

void Editor::UI::MeshWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext) {
    if (!entity.HasComponent<ECS::Components::MeshComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        auto *comp = entity.GetComponent<ECS::Components::MeshComponent>();

        if (comp->mesh) {
            ImGui::Text("Factory: %s",
                        comp->mesh->GetFactoryId().empty() ? "Unknown" : comp->mesh->GetFactoryId().c_str());
            ImGui::Text("Indices: %u", comp->mesh->GetIndexCount());
        } else {
            ImGui::TextDisabled("No mesh assigned");
        }

        ImGui::Spacing();
        ImGui::SeparatorText("Asset");

        if (engineContext && engineContext->resources) {
            ImGui::PushID("MeshCombo");
            if (MeshCombo("Mesh", *engineContext->resources, comp->mesh)) {
                MarkSceneChanged(engineContext);
            }
            ImGui::PopID();
        }

        ImGui::Separator();
        const float buttonWidth = ImGui::CalcTextSize("Remove Mesh").x + ImGui::GetStyle().FramePadding.x * 2;
        const float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - buttonWidth) * 0.5f);
        if (ImGui::Button("Remove ##Mesh", ImVec2(buttonWidth, 0))) {
            entity.RemoveComponent<ECS::Components::MeshComponent>();
            MarkSceneChanged(engineContext);
        }
    }
}

//  MaterialWidget

const char *Editor::UI::MaterialWidget::GetName() const { return "Material"; }

void Editor::UI::MaterialWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext) {
    if (!entity.HasComponent<ECS::Components::MaterialComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        auto *comp = entity.GetComponent<ECS::Components::MaterialComponent>();

        if (comp->material) {
            {
                ImGui::PushID("MatColor");
                ImGui::ColorEdit4("Color", &comp->material->color.x, ImGuiColorEditFlags_NoInputs);
                if (ImGui::IsItemDeactivatedAfterEdit())
                    MarkSceneChanged(engineContext);
                ImGui::PopID();
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Texture");

            if (engineContext && engineContext->resources) {
                ImGui::PushID("TextureCombo");
                if (TextureCombo("Texture", *engineContext->resources, comp->material->texture)) {
                    MarkSceneChanged(engineContext);
                }
                ImGui::PopID();
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Shader");

            if (engineContext && engineContext->resources) {
                ImGui::PushID("ShaderCombo");
                if (ShaderCombo("Shader", *engineContext->resources, comp->material->shader)) {
                    MarkSceneChanged(engineContext);
                }
                ImGui::PopID();
            }

            if (comp->material->shader) {
                ImGui::Text("Vert: %s", comp->material->shader->GetVertexPath().c_str());
                ImGui::Text("Frag: %s", comp->material->shader->GetFragmentPath().c_str());
            }

            // shader reload is not needed for texture changes.
        } else {
            ImGui::TextDisabled("No material assigned");
            if (engineContext && engineContext->resources) {
                ImGui::PushID("AssignMaterialCombo");
                if (MaterialCombo("Assign Material", *engineContext->resources, comp->material)) {
                    MarkSceneChanged(engineContext);
                }
                ImGui::PopID();
            }
        }

        ImGui::Separator();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                             (ImGui::GetContentRegionAvail().x -
                              (ImGui::CalcTextSize("Remove Material").x + ImGui::GetStyle().FramePadding.x * 2)) *
                                     0.5f);
        if (ImGui::Button("Remove ##Material")) {
            entity.RemoveComponent<ECS::Components::MaterialComponent>();
            MarkSceneChanged(engineContext);
        }
    }
}

//  DirectionalTextureWidget

const char *Editor::UI::DirectionalTextureWidget::GetName() const { return "Directional Texture"; }

void Editor::UI::DirectionalTextureWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext) {
    if (!entity.HasComponent<ECS::Components::DirectionalTextureComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        auto *comp = entity.GetComponent<ECS::Components::DirectionalTextureComponent>();

        ImGui::SliderInt("Facing Index", &comp->index, 0, 5);
        if (ImGui::IsItemDeactivatedAfterEdit())
            MarkSceneChanged(engineContext);
        ImGui::Spacing();

        if (engineContext && engineContext->resources) {
            for (int i = 0; i < 6; i++) {
                ImGui::PushID(i);
                char label[32];
                snprintf(label, sizeof(label), "Dir %d Texture", i);
                if (TextureCombo(label, *engineContext->resources, comp->textures[i])) {
                    MarkSceneChanged(engineContext);
                }
                ImGui::PopID();
            }
        }

        ImGui::Separator();
        const float buttonWidth =
                ImGui::CalcTextSize("Remove Directional Texture").x + ImGui::GetStyle().FramePadding.x * 2;
        const float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - buttonWidth) * 0.5f);
        if (ImGui::Button("Remove ##DirectionalTexture", ImVec2(buttonWidth, 0))) {
            entity.RemoveComponent<ECS::Components::DirectionalTextureComponent>();
            MarkSceneChanged(engineContext);
        }
    }
}

//  MapWidget

const char *Editor::UI::MapWidget::GetName() const { return "Map"; }

void Editor::UI::MapWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext) {
    if (!entity.HasComponent<ECS::Components::MapComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        auto *comp = entity.GetComponent<ECS::Components::MapComponent>();

        ImGui::Spacing();
        ImGui::SeparatorText("Map File");

        ImGui::PushID("MapFileCombo");
        if (FileCombo("Map File", std::string(Core::MAP_PATH), std::string(Core::MAP_FILE_EXTENSION),
                      comp->mapFilePath)) {
            MarkSceneChanged(engineContext);
        }
        ImGui::PopID();

        ImGui::Checkbox("Needs Mesh Update", &comp->needsMeshUpdate);
        if (ImGui::IsItemDeactivatedAfterEdit())
            MarkSceneChanged(engineContext);
        ImGui::Text("Render Visibles: %zu types", comp->visibles.size());
        ImGui::Text("Hex Mesh: %s", comp->hexMesh ? "Loaded" : "Missing");

        ImGui::Separator();
        const float buttonWidth = ImGui::CalcTextSize("Remove Map").x + ImGui::GetStyle().FramePadding.x * 2;
        const float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - buttonWidth) * 0.5f);
        if (ImGui::Button("Remove ##Map", ImVec2(buttonWidth, 0))) {
            entity.RemoveComponent<ECS::Components::MapComponent>();
            MarkSceneChanged(engineContext);
        }
    }
}

//  MapStateWidget

const char *Editor::UI::MapStateWidget::GetName() const { return "Map State"; }

void Editor::UI::MapStateWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext) {
    if (!entity.HasComponent<ECS::Components::MapStateComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        auto *comp = entity.GetComponent<ECS::Components::MapStateComponent>();
        ImGui::Checkbox("Has Selection", &comp->hasSelection);
        if (ImGui::IsItemDeactivatedAfterEdit())
            MarkSceneChanged(engineContext);
        if (comp->hasSelection)
            ImGui::Text("Selected Hex: [%d, %d]", comp->selectedHex.q, comp->selectedHex.r);
        ImGui::Checkbox("Has Path To", &comp->hasPathTo);
        if (ImGui::IsItemDeactivatedAfterEdit())
            MarkSceneChanged(engineContext);
        if (comp->hasPathTo)
            ImGui::Text("Path Target: [%d, %d]", comp->pathTo.q, comp->pathTo.r);

        ImGui::Separator();
        const float buttonWidth = ImGui::CalcTextSize("Remove Map State").x + ImGui::GetStyle().FramePadding.x * 2;
        const float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - buttonWidth) * 0.5f);
        if (ImGui::Button("Remove ##MapState", ImVec2(buttonWidth, 0))) {
            entity.RemoveComponent<ECS::Components::MapStateComponent>();
            MarkSceneChanged(engineContext);
        }
    }
}

//  ScriptWidget

const char *Editor::UI::ScriptWidget::GetName() const { return "Scripts"; }

void Editor::UI::ScriptWidget::Draw(ECS::Entity entity, Core::EngineContext *engineContext) {
    if (!entity.HasComponent<ECS::Components::ScriptComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        auto *comp = entity.GetComponent<ECS::Components::ScriptComponent>();

        for (size_t i = 0; i < comp->scriptPaths.size(); i++) {
            ImGui::PushID(static_cast<int>(i));
            ImGui::BulletText("%s", comp->scriptPaths[i].c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove ##Script")) {
                // remove this script from all vectors
                comp->scriptPaths.erase(comp->scriptPaths.begin() + i);
                comp->instance_envs.erase(comp->instance_envs.begin() + i);
                comp->on_update_functions.erase(comp->on_update_functions.begin() + i);
                comp->on_destroy_functions.erase(comp->on_destroy_functions.begin() + i);
                comp->on_exit_functions.erase(comp->on_exit_functions.begin() + i);
                comp->isInitialized.erase(comp->isInitialized.begin() + i);
                comp->source_codes.erase(comp->source_codes.begin() + i);
                comp->ast_nodes.erase(comp->ast_nodes.begin() + i);
                comp->lastModified.erase(comp->lastModified.begin() + i);

                // if no scripts then remove the entire component
                if (comp->scriptPaths.empty()) {
                    entity.RemoveComponent<ECS::Components::ScriptComponent>();
                }
                MarkSceneChanged(engineContext);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }

        ImGui::Separator();

        // add script picks from existing VFS scripts, maybe update to also auto-import from file
        // idk whats best?
        if (engineContext) {
            static std::string pendingScriptPath;
            ImGui::PushID("AddScriptCombo");
            if (FileCombo("Add Script", std::string(Core::SCRIPT_PATH), std::string(Core::SCRIPT_FILE_EXTENSION),
                          pendingScriptPath)) {
                if (!pendingScriptPath.empty()) {
                    // ensure component exists
                    if (!entity.HasComponent<ECS::Components::ScriptComponent>()) {
                        entity.AddComponent<ECS::Components::ScriptComponent>();
                        comp = entity.GetComponent<ECS::Components::ScriptComponent>();
                    }
                    // add the script path
                    comp->scriptPaths.push_back(pendingScriptPath);
                    comp->instance_envs.emplace_back();
                    comp->on_update_functions.emplace_back();
                    comp->on_destroy_functions.emplace_back();
                    comp->on_exit_functions.emplace_back();
                    comp->isInitialized.push_back(false);
                    comp->source_codes.emplace_back();
                    comp->ast_nodes.emplace_back();
                    comp->lastModified.push_back(std::filesystem::file_time_type::min());
                    MarkSceneChanged(engineContext);
                    pendingScriptPath.clear();
                }
            }
            ImGui::PopID();
        }

        // total count
        ImGui::SameLine();
        ImGui::TextDisabled("Total: %zu script(s)", comp->scriptPaths.size());
    }
}

//  CustomDataWidget

const char *Editor::UI::CustomDataWidget::GetName() const { return "ObSL Custom Data"; }

void Editor::UI::CustomDataWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext) {
    if (!entity.HasComponent<ECS::Components::CustomDataComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        auto *comp = entity.GetComponent<ECS::Components::CustomDataComponent>();
        if (comp->script_components.empty()) {
            ImGui::TextDisabled("No script variables defined.");
        } else {
            for (const auto &varName : comp->script_components | std::views::keys) {
                ImGui::BulletText("%s", varName.c_str());
            }
        }

        ImGui::Separator();
        const float buttonWidth =
                ImGui::CalcTextSize("Remove ObSL Custom Data").x + ImGui::GetStyle().FramePadding.x * 2;
        const float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - buttonWidth) * 0.5f);
        if (ImGui::Button("Remove ##CustomData", ImVec2(buttonWidth, 0))) {
            entity.RemoveComponent<ECS::Components::CustomDataComponent>();
            MarkSceneChanged(engineContext);
        }
    }
}
