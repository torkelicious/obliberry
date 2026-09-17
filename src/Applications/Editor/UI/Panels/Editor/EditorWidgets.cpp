#include "EditorWidgets.h"
#include "Applications/Editor/Commands/EditorCommands.h"
#include "Applications/Editor/EditorLayer.h"
#include <algorithm>
#include <cfloat>
#include <cstring>
#include <memory>
#include <type_traits>
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/SpriteAnimatorComponent.h"
#include "ECS/Entity.h"
#include "ECS/Systems/Animation/Types.h"
#include "ECS/Types.h"
#include "EditorWidgetsCombo.h"
#include "IO/Loaders/ParticleEmitterPrefabManager.h"
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
#include "IO/VFS/VFS.h"
#include "Rendering/Types/Shader/Shader.h"
#include "imgui.h"
#include "ECS/Components/SpriteSheetComponent.h"
#include "ECS/Systems/Animation/Animation.h"

#include <filesystem>


//  PointLightWidget

Editor::UI::PointLightWidget::PointLightWidget() : AutoComponentWidget("Point Light") {
    m_Fields.push_back({.Name = "Color", .Type = FieldType::Color3, .Offset = offsetof(ECS::Components::PointLightComponent, color)});
    m_Fields.push_back({.Name = "Radius", .Type = FieldType::Float, .Offset = offsetof(ECS::Components::PointLightComponent, radius)});
    m_Fields.push_back({.Name = "Intensity", .Type = FieldType::Float, .Offset = offsetof(ECS::Components::PointLightComponent, intensity)});
    m_DirtyOffset = offsetof(ECS::Components::PointLightComponent, dirty);
}

//  TransformWidget

Editor::UI::TransformWidget::TransformWidget() : AutoComponentWidget("Transform") {}

void Editor::UI::TransformWidget::DrawExtras(const ECS::Entity entity, ECS::Components::TransformComponent *component, Core::EngineContext *engineContext, UndoManager *undoManager) {
    auto pos = component->transform.GetPosition();
    static glm::vec3 s_DragStartPos;
    if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
        component->transform.SetPosition(pos);
    }
    if (ImGui::IsItemActivated())
        s_DragStartPos = component->transform.GetPosition();
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        undoManager->Execute(std::make_unique<Commands::TranslateEntityCommand>(static_cast<ECS::EntityID>(entity), s_DragStartPos, pos), *engineContext);
        MarkSceneChanged(engineContext);
    }

    const bool isBillboard = entity.HasComponent<ECS::Components::BillboardTagComponent>();

    if (isBillboard) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " Rotation has no effect on Billboard Sprites");
        ImGui::BeginDisabled();
    }

    auto rot = component->transform.GetRotation();
    static glm::vec3 s_DragStartRot;
    if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f)) {
        component->transform.SetRotation(rot);
    }
    if (ImGui::IsItemActivated())
        s_DragStartRot = component->transform.GetRotation();

    if (isBillboard) {
        ImGui::EndDisabled();
    }

    if (ImGui::IsItemDeactivatedAfterEdit()) {
        undoManager->Execute(std::make_unique<Commands::RotateEntityCommand>(static_cast<ECS::EntityID>(entity), s_DragStartRot, rot), *engineContext);
        MarkSceneChanged(engineContext);
    }

    auto scale = component->transform.GetScale();
    static glm::vec3 s_DragStartScale;
    if (ImGui::DragFloat3("Scale", &scale.x, 0.1f)) {
        component->transform.SetScale(scale);
    }
    if (ImGui::IsItemActivated())
        s_DragStartScale = component->transform.GetScale();
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        undoManager->Execute(std::make_unique<Commands::ScaleEntityCommand>(static_cast<ECS::EntityID>(entity), s_DragStartScale, scale), *engineContext);
        MarkSceneChanged(engineContext);
    }

    ImGui::Spacing();
    const bool hasBillboard = entity.HasComponent<ECS::Components::BillboardTagComponent>();
    bool useBillboard = hasBillboard;

    if (ImGui::Checkbox("Use Billboard", &useBillboard)) {
        const auto entId = static_cast<ECS::EntityID>(entity);
        if (useBillboard && !hasBillboard) {
            undoManager->Execute(std::make_unique<Commands::AddComponentCommand<ECS::Components::BillboardTagComponent>>(entId, ECS::Components::BillboardTagComponent{}), *engineContext);
        } else if (!useBillboard && hasBillboard) {
            undoManager->Execute(std::make_unique<Commands::RemoveComponentCommand<ECS::Components::BillboardTagComponent>>(entId, ECS::Components::BillboardTagComponent{}), *engineContext);
        }
        MarkSceneChanged(engineContext);
    }
}

//  MovementWidget

Editor::UI::MovementWidget::MovementWidget() : AutoComponentWidget("Movement") {
    m_Fields.push_back({.Name = "Time Per Step", .Type = FieldType::Float, .Offset = offsetof(ECS::Components::MovementComponent, timePerStep)});
    m_Fields.push_back({.Name = "Step Timer", .Type = FieldType::Float, .Offset = offsetof(ECS::Components::MovementComponent, stepTimer)});
    m_Fields.push_back({.Name = "Idle Timer", .Type = FieldType::Float, .Offset = offsetof(ECS::Components::MovementComponent, idleTimer)});
    m_Fields.push_back({.Name = "Is Moving", .Type = FieldType::Bool, .Offset = offsetof(ECS::Components::MovementComponent, isMoving)});
    m_Fields.push_back({.Name = "Auto-move (use 'ai' system)", .Type = FieldType::Bool, .Offset = offsetof(ECS::Components::MovementComponent, autoMove)});
}

void Editor::UI::MovementWidget::DrawExtras(ECS::Entity entity, ECS::Components::MovementComponent *component, Core::EngineContext *engineContext, UndoManager *undoManager) {
    ImGui::Text("Path Nodes: %zu", component->currentPath.size());
    ImGui::Text("Current Path Index: %zu", component->currentPathIndex);
}

//  MeshWidget

const char *Editor::UI::MeshWidget::GetName() const { return "Mesh"; }

void Editor::UI::MeshWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext, UndoManager *undoManager) {
    if (!entity.HasComponent<ECS::Components::MeshComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        auto *comp = entity.GetComponent<ECS::Components::MeshComponent>();

        if (comp->mesh) {
            ImGui::Text("Factory: %s", comp->mesh->GetFactoryId().empty() ? "Unknown" : comp->mesh->GetFactoryId().c_str());
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
            auto data = *entity.GetComponent<ECS::Components::MeshComponent>();
            undoManager->Execute(std::make_unique<Commands::RemoveComponentCommand<ECS::Components::MeshComponent>>(static_cast<ECS::EntityID>(entity), data), *engineContext);
            MarkSceneChanged(engineContext);
        }
    }
}

//  MaterialWidget

const char *Editor::UI::MaterialWidget::GetName() const { return "Material"; }

void Editor::UI::MaterialWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext, UndoManager *undoManager) {
    if (!entity.HasComponent<ECS::Components::MaterialComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {

        if (engineContext && engineContext->resources) {
            auto *matComp = entity.GetComponent<ECS::Components::MaterialComponent>();
            ImGui::PushID("MatSelectCombo");
            if (MaterialCombo("Material", *engineContext->resources, matComp->material)) {
                MarkSceneChanged(engineContext);
            }
            ImGui::PopID();
        }

        if (auto *comp = entity.GetComponent<ECS::Components::MaterialComponent>(); comp->material) {
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
                const auto &sh = *comp->material->shader;
                ImGui::Text("Vert: %s", sh.GetVertexPath().empty() ? sh.GetDebugName().c_str() : sh.GetVertexPath().c_str());
                ImGui::Text("Frag: %s", sh.GetFragmentPath().empty() ? sh.GetDebugName().c_str() : sh.GetFragmentPath().c_str());
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "No shader assigned. Material will not render.");
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

        ImGui::Spacing();
        auto *matComp = entity.GetComponent<ECS::Components::MaterialComponent>();
        const float cloneWidth = ImGui::CalcTextSize("Clone Material").x + ImGui::GetStyle().FramePadding.x * 2;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - cloneWidth) * 0.5f);
        if (ImGui::Button("Clone Material", ImVec2(cloneWidth, 0))) {
            if (matComp && matComp->material && engineContext && engineContext->resources) {
                auto &resources = *engineContext->resources;
                const std::string origKey = resources.GetKey<Rendering::Material>(matComp->material);
                std::string newKey = origKey.empty() ? "material_clone" : origKey + "_clone";
                // ensure unique key
                int suffix = 1;
                while (resources.Get<Rendering::Material>(newKey)) {
                    newKey = (origKey.empty() ? "material_clone" : origKey + "_clone") + "_" + std::to_string(suffix++);
                }
                const auto cloned = resources.Load<Rendering::Material>(newKey, matComp->material->shader, matComp->material->texture, matComp->material->color);
                matComp->material = cloned;
                MarkSceneChanged(engineContext);
            }
        }

        ImGui::Separator();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - (ImGui::CalcTextSize("Remove Material").x + ImGui::GetStyle().FramePadding.x * 2)) * 0.5f);
        if (ImGui::Button("Remove ##Material")) {
            auto data = *entity.GetComponent<ECS::Components::MaterialComponent>();
            undoManager->Execute(std::make_unique<Commands::RemoveComponentCommand<ECS::Components::MaterialComponent>>(static_cast<ECS::EntityID>(entity), data), *engineContext);
            MarkSceneChanged(engineContext);
        }
    }
}

//  DirectionalTextureWidget

const char *Editor::UI::DirectionalTextureWidget::GetName() const { return "Directional Texture"; }

void Editor::UI::DirectionalTextureWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext, UndoManager *undoManager) {
    if (!entity.HasComponent<ECS::Components::DirectionalTextureComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        auto *comp = entity.GetComponent<ECS::Components::DirectionalTextureComponent>();

        int facingIndex = comp->index;
        static int s_DirTexOldIndex = 0;
        ImGui::SliderInt("Facing Index", &facingIndex, 0, 5);
        if (ImGui::IsItemActivated())
            s_DirTexOldIndex = static_cast<int>(comp->index);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            comp->index = static_cast<uint8_t>(facingIndex);
            if (undoManager && engineContext) {
                const auto entId = static_cast<ECS::EntityID>(entity);
                undoManager->Execute(std::make_unique<Commands::ModifyComponentFieldCommand<ECS::Components::DirectionalTextureComponent>>(entId, offsetof(ECS::Components::DirectionalTextureComponent, index),
                                                                                                                                           sizeof(uint8_t), &s_DirTexOldIndex, &comp->index, "Facing Index"),
                                     *engineContext);
            }
            MarkSceneChanged(engineContext);
        }
        ImGui::Spacing();

        if (engineContext && engineContext->resources) {
            int missingCount = 0;
            for (int i = 0; i < 6; i++) {
                ImGui::PushID(i);
                char label[32];
                snprintf(label, sizeof(label), "Dir %d Texture", i);
                if (TextureCombo(label, *engineContext->resources, comp->textures[i])) {
                    MarkSceneChanged(engineContext);
                }
                if (!comp->textures[i])
                    missingCount++;
                ImGui::PopID();
            }
            if (missingCount > 0) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%d direction(s) have no texture.", missingCount);
            }
        }

        ImGui::Separator();
        const float buttonWidth = ImGui::CalcTextSize("Remove Directional Texture").x + ImGui::GetStyle().FramePadding.x * 2;
        const float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - buttonWidth) * 0.5f);
        if (ImGui::Button("Remove ##DirectionalTexture", ImVec2(buttonWidth, 0))) {
            auto data = *entity.GetComponent<ECS::Components::DirectionalTextureComponent>();
            undoManager->Execute(std::make_unique<Commands::RemoveComponentCommand<ECS::Components::DirectionalTextureComponent>>(static_cast<ECS::EntityID>(entity), data), *engineContext);
            MarkSceneChanged(engineContext);
        }
    }
}

//  MapWidget

const char *Editor::UI::MapWidget::GetName() const { return "Map"; }

void Editor::UI::MapWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext, UndoManager *undoManager) {
    if (!entity.HasComponent<ECS::Components::MapComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        auto *comp = entity.GetComponent<ECS::Components::MapComponent>();

        ImGui::Spacing();
        ImGui::SeparatorText("Map File");

        ImGui::PushID("MapFileCombo");
        if (FileCombo("Map File", std::string(Core::MAP_PATH), std::string(Core::MAP_FILE_EXTENSION), comp->mapFilePath)) {
            MarkSceneChanged(engineContext);
        }
        ImGui::PopID();

        ImGui::Checkbox("Needs Mesh Update", &comp->needsMeshUpdate);
        if (ImGui::IsItemDeactivatedAfterEdit())
            MarkSceneChanged(engineContext);
        ImGui::Text("Render Visibles: %zu types", comp->visibles.size());
        ImGui::Text("Tile Types: %zu", comp->typeMats.size());
        ImGui::Text("Hex Mesh: %s", comp->hexMesh ? "Loaded" : "Missing");
        if (!comp->outlineMat || !comp->pathToMat) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Overlay materials missing. Selection/path highlights will not render.");
        }

        ImGui::Separator();
        const float buttonWidth = ImGui::CalcTextSize("Remove Map").x + ImGui::GetStyle().FramePadding.x * 2;
        const float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - buttonWidth) * 0.5f);
        if (ImGui::Button("Remove ##Map", ImVec2(buttonWidth, 0))) {
            auto data = *entity.GetComponent<ECS::Components::MapComponent>();
            undoManager->Execute(std::make_unique<Commands::RemoveComponentCommand<ECS::Components::MapComponent>>(static_cast<ECS::EntityID>(entity), data), *engineContext);
            MarkSceneChanged(engineContext);
        }
    }
}

//  MapStateWidget

const char *Editor::UI::MapStateWidget::GetName() const { return "Map State"; }

void Editor::UI::MapStateWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext, UndoManager *undoManager) {
    if (!entity.HasComponent<ECS::Components::MapStateComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        auto *comp = entity.GetComponent<ECS::Components::MapStateComponent>();
        const auto entId = static_cast<ECS::EntityID>(entity);

        {
            static bool s_OldHasSelection = false;
            bool hasSel = comp->hasSelection;
            ImGui::Checkbox("Has Selection", &hasSel);
            if (ImGui::IsItemActivated())
                s_OldHasSelection = comp->hasSelection;
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                comp->hasSelection = hasSel;
                if (undoManager && engineContext)
                    undoManager->Execute(std::make_unique<Commands::ModifyComponentFieldCommand<ECS::Components::MapStateComponent>>(entId, offsetof(ECS::Components::MapStateComponent, hasSelection), sizeof(bool),
                                                                                                                                     &s_OldHasSelection, &comp->hasSelection, "Has Selection"),
                                         *engineContext);
                MarkSceneChanged(engineContext);
            }
        }
        if (comp->hasSelection)
            ImGui::Text("Selected Hex: [%d, %d]", comp->selectedHex.q, comp->selectedHex.r);

        {
            static bool s_OldHasPathTo = false;
            bool hasPath = comp->hasPathTo;
            ImGui::Checkbox("Has Path To", &hasPath);
            if (ImGui::IsItemActivated())
                s_OldHasPathTo = comp->hasPathTo;
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                comp->hasPathTo = hasPath;
                if (undoManager && engineContext)
                    undoManager->Execute(std::make_unique<Commands::ModifyComponentFieldCommand<ECS::Components::MapStateComponent>>(entId, offsetof(ECS::Components::MapStateComponent, hasPathTo), sizeof(bool),
                                                                                                                                     &s_OldHasPathTo, &comp->hasPathTo, "Has Path To"),
                                         *engineContext);
                MarkSceneChanged(engineContext);
            }
        }
        if (comp->hasPathTo)
            ImGui::Text("Path Target: [%d, %d]", comp->pathTo.q, comp->pathTo.r);

        ImGui::Separator();
        const float buttonWidth = ImGui::CalcTextSize("Remove Map State").x + ImGui::GetStyle().FramePadding.x * 2;
        const float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - buttonWidth) * 0.5f);
        if (ImGui::Button("Remove ##MapState", ImVec2(buttonWidth, 0))) {
            auto data = *entity.GetComponent<ECS::Components::MapStateComponent>();
            undoManager->Execute(std::make_unique<Commands::RemoveComponentCommand<ECS::Components::MapStateComponent>>(static_cast<ECS::EntityID>(entity), data), *engineContext);
            MarkSceneChanged(engineContext);
        }
    }
}

//  ScriptWidget

const char *Editor::UI::ScriptWidget::GetName() const { return "Scripts"; }

void Editor::UI::ScriptWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext, UndoManager *undoManager) {
    if (!entity.HasComponent<ECS::Components::ScriptComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        auto *comp = entity.GetComponent<ECS::Components::ScriptComponent>();

        for (size_t i = 0; i < comp->slots.size(); i++) {
            ImGui::PushID(static_cast<int>(i));
            ImGui::BulletText("%s", comp->slots[i].scriptPath.c_str());
            // ImGui::SameLine();
            if (ImGui::SmallButton("Remove ##Script")) {
                undoManager->Execute(std::make_unique<Commands::RemoveScriptCommand>(static_cast<ECS::EntityID>(entity), *comp, static_cast<int>(i)), *engineContext);
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
            if (FileCombo("Add Script", std::string(Core::SCRIPT_PATH), std::string(Core::SCRIPT_FILE_EXTENSION), pendingScriptPath)) {
                undoManager->Execute(std::make_unique<Commands::AddScriptCommand>(static_cast<ECS::EntityID>(entity), pendingScriptPath), *engineContext);
                pendingScriptPath.clear();
            }
            ImGui::PopID();
        }

        // total count
        ImGui::SameLine();
        ImGui::TextDisabled("Total: %zu script(s)", comp->slots.size());
    }
}

//  CustomDataWidget

const char *Editor::UI::CustomDataWidget::GetName() const { return "ObSL Custom Data"; }

void Editor::UI::CustomDataWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext, UndoManager *undoManager) {
    if (!entity.HasComponent<ECS::Components::CustomDataComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        if (auto *comp = entity.GetComponent<ECS::Components::CustomDataComponent>(); comp->script_components.empty()) {
            ImGui::TextDisabled("No script variables defined.");
        } else {
            for (const auto &varName : comp->script_components | std::views::keys) {
                ImGui::BulletText("%s", varName.c_str());
            }
        }

        ImGui::Separator();
        const float buttonWidth = ImGui::CalcTextSize("Remove ObSL Custom Data").x + ImGui::GetStyle().FramePadding.x * 2;
        const float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - buttonWidth) * 0.5f);
        if (ImGui::Button("Remove ##CustomData", ImVec2(buttonWidth, 0))) {
            auto data = *entity.GetComponent<ECS::Components::CustomDataComponent>();
            undoManager->Execute(std::make_unique<Commands::RemoveComponentCommand<ECS::Components::CustomDataComponent>>(static_cast<ECS::EntityID>(entity), data), *engineContext);
            MarkSceneChanged(engineContext);
        }
    }
}

//  ParticleEmitterWidget

const char *Editor::UI::ParticleEmitterWidget::GetName() const { return "Particle Emitter"; }

void Editor::UI::ParticleEmitterWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext, UndoManager *undoManager) {
    if (!entity.HasComponent<ECS::Components::ParticleEmitterComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        auto *comp = entity.GetComponent<ECS::Components::ParticleEmitterComponent>();

        ImGui::SeparatorText("Emission");
        ImGui::Checkbox("Active", &comp->active);
        ImGui::SameLine();
        ImGui::Checkbox("Editor Preview", &EditorLayer::s_RenderParticlesInEditor);

        ImGui::DragInt("Max Particles", &comp->maxParticles, 1.0f, 1, 16384);
        ImGui::DragFloat("Emit Rate", &comp->emitRate, 1.0f, 0.0f, 1000.0f, "%.1f/s");
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            comp->isDirty = true;
            MarkSceneChanged(engineContext);
        }

        ImGui::SeparatorText("Lifetime");
        ImGui::DragFloat("Min", &comp->lifetimeMin, 0.05f, 0.01f, 60.0f, "%.2f s");
        ImGui::DragFloat("Max", &comp->lifetimeMax, 0.05f, 0.01f, 60.0f, "%.2f s");
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            comp->isDirty = true;
            MarkSceneChanged(engineContext);
        }

        ImGui::SeparatorText("Velocity");
        ImGui::DragFloat3("Min", &comp->velocityMin.x, 0.1f);
        ImGui::DragFloat3("Max", &comp->velocityMax.x, 0.1f);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            comp->isDirty = true;
            MarkSceneChanged(engineContext);
        }

        ImGui::SeparatorText("Physics");
        ImGui::DragFloat3("Gravity", &comp->gravity.x, 0.1f);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            comp->isDirty = true;
            MarkSceneChanged(engineContext);
        }

        ImGui::SeparatorText("Size");
        ImGui::DragFloat("Start Min", &comp->sizeStartMin, 0.01f, 0.001f, 50.0f, "%.3f");
        ImGui::DragFloat("Start Max", &comp->sizeStartMax, 0.01f, 0.001f, 50.0f, "%.3f");
        ImGui::DragFloat("End Min", &comp->sizeEndMin, 0.01f, 0.0f, 50.0f, "%.3f");
        ImGui::DragFloat("End Max", &comp->sizeEndMax, 0.01f, 0.0f, 50.0f, "%.3f");
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            comp->isDirty = true;
            MarkSceneChanged(engineContext);
        }

        ImGui::SeparatorText("Rotation");
        ImGui::DragFloat("Speed Min", &comp->rotationSpeedMin, 0.1f, -100.0f, 100.0f, "%.2f");
        ImGui::DragFloat("Speed Max", &comp->rotationSpeedMax, 0.1f, -100.0f, 100.0f, "%.2f");
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            comp->isDirty = true;
            MarkSceneChanged(engineContext);
        }

        ImGui::SeparatorText("Color");
        ImGui::ColorEdit4("Start", &comp->colorStart.x, ImGuiColorEditFlags_AlphaBar);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            comp->isDirty = true;
            MarkSceneChanged(engineContext);
        }
        ImGui::ColorEdit4("End", &comp->colorEnd.x, ImGuiColorEditFlags_AlphaBar);
        if (ImGui::IsItemDeactivatedAfterEdit())
            MarkSceneChanged(engineContext);

        ImGui::SeparatorText("Options");
        ImGui::Checkbox("Billboard", &comp->isBillboard);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            comp->isDirty = true;
            MarkSceneChanged(engineContext);
        }
        {
            const char *blendModes[] = {"Alpha", "Additive"};
            int bm = static_cast<int>(comp->blendMode);
            if (ImGui::Combo("Blend Mode", &bm, blendModes, 2)) {
                comp->blendMode = static_cast<ECS::Components::ParticleBlendMode>(bm);
                comp->isDirty = true;
                MarkSceneChanged(engineContext);
            }
        }
        ImGui::DragInt("Render Order", &comp->renderOrder, 0.1f, -10, 10);
        if (ImGui::IsItemDeactivatedAfterEdit())
            comp->isDirty = true;
        MarkSceneChanged(engineContext);
        {
            const char *shapes[] = {"Quad", "Circle", "Soft Circle"};
            if (ImGui::Combo("Shape", &comp->shape, shapes, 3)) {
                comp->isDirty = true;
                MarkSceneChanged(engineContext);
            }
        }

        ImGui::SeparatorText("Material");
        if (engineContext && engineContext->resources) {
            ImGui::PushID("EmitterMatCombo");
            if (MaterialCombo("Material", *engineContext->resources, comp->material)) {
                comp->isDirty = true;
                MarkSceneChanged(engineContext);
            }
            ImGui::PopID();
        }
        if (!comp->material) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "No material assigned. Particles will not render.");
        } else if (!comp->material->texture) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Material has no texture. Particles will render as solid color.");
        }

        // Preset: Load / Save
        ImGui::SeparatorText("Presets");
        {
            static std::string pendingPresetPath;
            ImGui::PushID("LoadEmitterPreset");
            if (FileCombo("Load Preset", std::string(Core::PARTICLE_PRESET_PATH), std::string(".json"), pendingPresetPath)) {
                if (!pendingPresetPath.empty()) {
                    if (const auto preset = IO::LoadEmitterPreset(pendingPresetPath)) {
                        *comp = *preset;
                        comp->isDirty = true;
                        MarkSceneChanged(engineContext);
                    }
                }
                pendingPresetPath.clear();
            }
            ImGui::PopID();
        }
        {
            if (ImGui::Button("Save as Preset")) {
                ImGui::OpenPopup("SaveEmitterPresetPopup");
            }
            if (ImGui::BeginPopup("SaveEmitterPresetPopup")) {
                static char presetNameBuf[128] = "";
                if (ImGui::IsWindowAppearing()) {
                    presetNameBuf[0] = '\0';
                }
                ImGui::InputText("Name", presetNameBuf, sizeof(presetNameBuf));
                if (ImGui::Button("Save") && presetNameBuf[0] != '\0') {
                    const std::string filepath = std::string(Core::PARTICLE_PRESET_PATH) + std::string(presetNameBuf) + ".json";
                    // ensure directory exists
                    const auto resolved = IO::VFS::Resolve(std::string(Core::PARTICLE_PRESET_PATH));
                    std::filesystem::create_directories(resolved);
                    IO::SerializeEmitter(*comp, filepath);
                    MarkSceneChanged(engineContext);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        ImGui::Separator();
        const float buttonWidth = ImGui::CalcTextSize("Remove Particle Emitter").x + ImGui::GetStyle().FramePadding.x * 2;
        const float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - buttonWidth) * 0.5f);
        if (ImGui::Button("Remove ##ParticleEmitter", ImVec2(buttonWidth, 0))) {
            auto data = *entity.GetComponent<ECS::Components::ParticleEmitterComponent>();
            undoManager->Execute(std::make_unique<Commands::RemoveComponentCommand<ECS::Components::ParticleEmitterComponent>>(static_cast<ECS::EntityID>(entity), data), *engineContext);
            MarkSceneChanged(engineContext);
        }
    }
}

const char *Editor::UI::ColliderWidget::GetName() const { return "Collider"; }

void Editor::UI::ColliderWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext, UndoManager *undoManager) {
    using Component = ECS::Components::ColliderComponent;

    static_assert(std::is_trivially_copyable_v<Component>);

    if (!entity.HasComponent<Component>())
        return;

    if (!ImGui::CollapsingHeader(GetName()))
        return;

    auto *c = entity.GetComponent<Component>();
    const auto id = static_cast<ECS::EntityID>(entity);

    auto commit = [&](const Component &before) {
        if (undoManager && engineContext) {
            undoManager->Execute(std::make_unique<Commands::ModifyComponentFieldCommand<Component>>(id, 0, sizeof(Component), &before, c, "Edit Collider"), *engineContext);
        }

        MarkSceneChanged(engineContext);
    };

    // snapshot used while a field is being edited; captured when it becomes active
    static Component s_BeforeEdit;

    auto field = [&](auto draw) {
        const Component before = *c;
        const bool changed = draw();

        if (ImGui::IsItemActivated())
            s_BeforeEdit = before;

        if (ImGui::IsItemDeactivatedAfterEdit())
            commit(s_BeforeEdit);
        else if (changed && !ImGui::IsItemActive())
            commit(before);

        if (changed)
            MarkSceneChanged(engineContext);
    };

    field([&] {
        int value = static_cast<int>(c->shape);
        const bool changed = ImGui::Combo("Shape", &value, "Box\0Sphere\0Cylinder\0Rectangle\0Circle\0");

        if (changed)
            c->shape = static_cast<ECS::Components::ColliderShape>(value);

        return changed;
    });

    field([&] {
        int value = static_cast<int>(c->orientation);
        const bool changed = ImGui::Combo("Orientation", &value, "Entity\0Billboard\0");

        if (changed)
            c->orientation = static_cast<ECS::Components::ColliderOrientation>(value);

        return changed;
    });

    field([&] { return ImGui::DragFloat3("Offset", &c->offset.x, 0.01f); });

    constexpr float minimum = 0.001f;
    constexpr auto flags = ImGuiSliderFlags_AlwaysClamp;

    if (c->shape == ECS::Components::ColliderShape::Box) {
        field([&] { return ImGui::DragFloat3("Size", &c->size.x, 0.01f, minimum, FLT_MAX, "%.3f", flags); });
    } else if (c->shape == ECS::Components::ColliderShape::Rectangle) {
        field([&] { return ImGui::DragFloat2("Size", &c->size.x, 0.01f, minimum, FLT_MAX, "%.3f", flags); });
    } else {
        field([&] { return ImGui::DragFloat("Radius", &c->radius, 0.01f, minimum, FLT_MAX, "%.3f", flags); });
        if (c->shape == ECS::Components::ColliderShape::Cylinder) {
            field([&] { return ImGui::DragFloat("Height", &c->height, 0.01f, minimum, FLT_MAX, "%.3f", flags); });
        }
    }

    field([&] { return ImGui::Checkbox("Trigger", &c->isTrigger); });

    ImGui::Separator();
    const float buttonWidth = ImGui::CalcTextSize("Remove Collider").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availableWidth - buttonWidth) * 0.5f);
    if (ImGui::Button("Remove ##Collider", ImVec2(buttonWidth, 0.0f))) {
        if (undoManager && engineContext) {
            undoManager->Execute(std::make_unique<Commands::RemoveComponentCommand<Component>>(id, *c), *engineContext);
        } else {
            entity.RemoveComponent<Component>();
        }

        MarkSceneChanged(engineContext);
    }
}

const char *Editor::UI::SpriteSheetWidget::GetName() const { return "Sprite Sheet"; }

void Editor::UI::SpriteSheetWidget::Draw(ECS::Entity entity, Core::EngineContext *ctx, UndoManager *undomgr) {
    using Sprite = ECS::Components::SpriteSheetComponent;
    using Player = ECS::Components::SpriteAnimatorComponent;

    auto *sprite = entity.GetComponent<Sprite>();
    if (!sprite) {
        m_HasDraft = false;
        return;
    }

    if (!ImGui::CollapsingHeader(GetName())) {
        return;
    }

    ImGui::PushID("SpriteSheetWidget");

    const auto id = static_cast<ECS::EntityID>(entity);
    auto *player = entity.GetComponent<Player>();
    auto &resources = Core::ResourceManager::GetInstance();

    if (!player && ImGui::Button("Add Animator")) {
        if (undomgr && ctx) {
            undomgr->Execute(std::make_unique<Commands::AddComponentCommand<Player>>(id, Player{}), *ctx);
        } else {
            entity.AddComponent<Player>();
        }

        m_HasDraft = false;
        MarkSceneChanged(ctx);

        ImGui::PopID();
        return;
    }

    if (player) {
        m_HasDraft = false;
        bool configChanged = false;

        auto assets = resources.GetAll<Animation::SpriteAnimationSet>();

        std::sort(assets.begin(), assets.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

        std::string selectedAsset = player->animations ? "<Unregistered>" : "<None>";

        for (const auto &[key, asset] : assets) {
            if (asset && asset == player->animations) {
                selectedAsset = key;
                break;
            }
        }

        if (ImGui::BeginCombo("Animation", selectedAsset.c_str())) {
            if (ImGui::Selectable("<None>", !player->animations)) {
                if (player->animations || !player->initialClip.empty()) {
                    player->animations.reset();
                    player->initialClip.clear();
                    configChanged = true;
                }
            }

            for (const auto &[key, asset] : assets) {
                if (!asset) {
                    continue;
                }

                const bool isSelected = asset == player->animations;

                ImGui::PushID(key.c_str());

                if (ImGui::Selectable(key.c_str(), isSelected)) {
                    if (!isSelected) {
                        player->animations = asset;

                        if (!asset->clips.contains(player->initialClip)) {
                            player->initialClip.clear();
                        }

                        configChanged = true;
                    }
                }

                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }

                ImGui::PopID();
            }

            ImGui::EndCombo();
        }

        if (player->animations) {
            std::vector<std::string> names;
            names.reserve(player->animations->clips.size());

            for (const auto &[name, clip] : player->animations->clips) {
                names.push_back(name);
            }

            std::sort(names.begin(), names.end());

            const char *preview = player->initialClip.empty() ? "<None>" : player->initialClip.c_str();

            if (ImGui::BeginCombo("Initial Clip", preview)) {
                if (ImGui::Selectable("<None>", player->initialClip.empty())) {
                    if (!player->initialClip.empty()) {
                        player->initialClip.clear();
                        configChanged = true;
                    }
                }

                for (const auto &name : names) {
                    const bool isSelected = name == player->initialClip;

                    ImGui::PushID(name.c_str());

                    if (ImGui::Selectable(name.c_str(), isSelected)) {
                        if (!isSelected) {
                            player->initialClip = name;
                            configChanged = true;
                        }
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }

                    ImGui::PopID();
                }

                ImGui::EndCombo();
            }
        }

        configChanged |= ImGui::Checkbox("Autoplay", &player->autoplay);

        if (configChanged) {
            Animation::ResetToInitial(*player);

            if (!Animation::ResolvePose(*player, *sprite)) {
                *sprite = Sprite{};
            }

            MarkSceneChanged(ctx);
        }

        if (!player->animations) {
            ImGui::TextDisabled("Choose an animation asset.");
        } else if (player->initialClip.empty()) {
            ImGui::TextDisabled("Choose an initial clip.");
        } else {
            const auto it = player->animations->clips.find(player->initialClip);

            if (it == player->animations->clips.end()) {
                ImGui::TextWrapped("The initial clip does not exist in this asset.");
            } else if (!Animation::ValidateClip(*player->animations, it->second)) {
                ImGui::TextWrapped("The initial clip has an invalid sheet, "
                                   "frame index, or frame duration.");
            }
        }

        ImGui::Text("Sheet Frame: %u", static_cast<unsigned int>(sprite->frame));
        ImGui::TextDisabled("The animator controls the sheet and frame.");

        if (ImGui::Button("Remove Animator")) {
            if (undomgr && ctx) {
                undomgr->Execute(std::make_unique<Commands::RemoveComponentCommand<Player>>(id, *player), *ctx);
            } else {
                entity.RemoveComponent<Player>();
            }

            m_HasDraft = false;
            MarkSceneChanged(ctx);
        }

        ImGui::PopID();
        return;
    }

    if (!m_HasDraft || m_EditEntity != entity || m_SourceSheet != sprite->sheet || m_SourceFrame != sprite->frame) {

        m_EditEntity = entity;
        m_SourceSheet = sprite->sheet;
        m_SourceFrame = sprite->frame;

        m_SheetDraft = sprite->sheet ? *sprite->sheet : Rendering::SpriteSheet{};

        m_FrameInput = static_cast<int>(std::min(sprite->frame, static_cast<uint32_t>(std::numeric_limits<int>::max())));

        m_HasDraft = true;
    }

    bool layoutChanged = TextureCombo("Texture", resources, m_SheetDraft.texture);

    layoutChanged |= ImGui::InputInt("Columns", &m_SheetDraft.columns);
    layoutChanged |= ImGui::InputInt("Rows", &m_SheetDraft.rows);

    layoutChanged |= ImGui::InputInt("Column spacing (px)", &m_SheetDraft.columnSpacing);

    layoutChanged |= ImGui::InputInt("Row spacing (px)", &m_SheetDraft.rowSpacing);

    const bool validLayout = Animation::ValidateSheet(m_SheetDraft);

    if (layoutChanged && validLayout) {
        sprite->sheet = std::make_shared<Rendering::SpriteSheet>(m_SheetDraft);

        const auto total = static_cast<uint32_t>(int64_t(m_SheetDraft.columns) * m_SheetDraft.rows);

        sprite->frame = std::min(sprite->frame, total - 1);

        m_FrameInput = static_cast<int>(sprite->frame);
        m_SourceSheet = sprite->sheet;
        m_SourceFrame = sprite->frame;

        MarkSceneChanged(ctx);
    }

    if (!m_SheetDraft.texture) {
        ImGui::TextDisabled("Choose a texture.");
    } else if (m_SheetDraft.columns <= 0 || m_SheetDraft.rows <= 0) {
        ImGui::TextWrapped("Columns and rows must be at least 1.");
    } else if (m_SheetDraft.columnSpacing < 0 || m_SheetDraft.rowSpacing < 0) {
        ImGui::TextWrapped("Spacing cannot be negative.");
    } else {
        const int64_t columns = m_SheetDraft.columns;
        const int64_t rows = m_SheetDraft.rows;

        const int64_t width = int64_t(m_SheetDraft.texture->GetWidth()) - int64_t(m_SheetDraft.columnSpacing) * (columns - 1);

        const int64_t height = int64_t(m_SheetDraft.texture->GetHeight()) - int64_t(m_SheetDraft.rowSpacing) * (rows - 1);

        if (columns * rows > std::numeric_limits<int>::max()) {
            ImGui::TextWrapped("The grid contains too many frames.");
        } else if (width < columns || height < rows) {
            ImGui::TextWrapped("The texture is too small for this grid and spacing. "
                               "Each frame needs at least one pixel in each dimension.");
        } else if (width % columns != 0 || height % rows != 0) {
            ImGui::TextWrapped("The grid produces %.3f x %.3f pixels per frame. "
                               "Both dimensions must be whole numbers.",
                               static_cast<double>(width) / columns, static_cast<double>(height) / rows);
        } else {
            ImGui::Text("Frame size: %lld x %lld px", static_cast<long long>(width / columns), static_cast<long long>(height / rows));
        }
    }

    if (!validLayout) {
        ImGui::TextWrapped("The last accepted layout remains active. "
                           "Spacing is between frames; it does not include an outer border.");
    }

    int64_t totalFrames = 0;

    if (sprite->sheet && Animation::ValidateSheet(*sprite->sheet)) {
        totalFrames = int64_t(sprite->sheet->columns) * sprite->sheet->rows;
    }

    ImGui::Text("Accepted sheet frames: %lld", static_cast<long long>(totalFrames));

    const bool frameChanged = ImGui::InputInt("Frame", &m_FrameInput);

    const bool validFrame = m_FrameInput >= 0 && int64_t(m_FrameInput) < totalFrames;

    if (frameChanged && validFrame) {
        sprite->frame = static_cast<uint32_t>(m_FrameInput);
        m_SourceFrame = sprite->frame;
        MarkSceneChanged(ctx);
    }

    if (totalFrames > 0 && !validFrame) {
        ImGui::TextWrapped("Frame must be between 0 and %lld.", static_cast<long long>(totalFrames - 1));
    }

    ImGui::Separator();

    if (ImGui::Button("Remove Sprite Sheet")) {
        if (undomgr && ctx) {
            undomgr->Execute(std::make_unique<Commands::RemoveComponentCommand<Sprite>>(id, *sprite), *ctx);
        } else {
            entity.RemoveComponent<Sprite>();
        }

        m_HasDraft = false;
        MarkSceneChanged(ctx);
    }

    ImGui::PopID();
}
