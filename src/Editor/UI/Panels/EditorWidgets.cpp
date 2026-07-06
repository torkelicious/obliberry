#include "EditorWidgets.h"
#include <cstring>

#include "Core/EngineContext.h"
#include "Editor/FileDialogs.h"
#include "ECS/Components/BillboardTagComponent.h"
#include "ECS/Components/CustomDataComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/ScriptComponent.h"
#include "ECS/Systems/ScriptSystem.h"
#include "IO/AssetLoader.h"
#include "IO/VFS.h"
#include "Rendering/MeshFactory.h"
#include "Rendering/Renderer.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include <filesystem>

namespace {
    // if path is inside the project root return the relative path.
    // otherwise import the asset via vfs
    std::optional<std::string> ResolveOrImportPath(const std::string &absolutePath,
                                                   const std::string &targetSubDir = "") {
        if (absolutePath.empty())
            return std::nullopt;

        const std::filesystem::path projectRoot = IO::VFS::GetProjectRoot();
        if (projectRoot.empty())
            return std::nullopt;

        std::error_code ec;
        std::filesystem::path absNorm = std::filesystem::absolute(absolutePath, ec);
        if (ec || absNorm.empty())
            return std::nullopt;
        absNorm = absNorm.lexically_normal();

        std::filesystem::path rootNorm = std::filesystem::absolute(projectRoot, ec);
        if (ec || rootNorm.empty())
            return std::nullopt;
        rootNorm = rootNorm.lexically_normal();

        for (auto p = absNorm; p.has_parent_path() && p != p.root_path(); p = p.parent_path()) {
            if (p == rootNorm) {
                // already inside project
                std::string rel = std::filesystem::proximate(absNorm, rootNorm).string();
                for (auto &c : rel)
                    if (c == '\\')
                        c = '/';
                return rel;
            }
        }
        return IO::AssetLoader::ImportAsset(absolutePath, targetSubDir);
    }
} // namespace

Editor::UI::PointLightWidget::PointLightWidget() : AutoComponentWidget("Point Light") {
    m_Fields.push_back({"Color", FieldType::Color3, offsetof(ECS::Components::PointLightComponent, color)});
    m_Fields.push_back({"Radius", FieldType::Float, offsetof(ECS::Components::PointLightComponent, radius)});
    m_Fields.push_back({"Intensity", FieldType::Float, offsetof(ECS::Components::PointLightComponent, intensity)});
}

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

        // Mesh factory selection
        constexpr const char *meshTypes[] = {"Quad",    "PointTopHex", "ETriang", "Ellipse", "Circle", "Pentagon",
                                             "Hexagon", "Octagon",     "Ring",    "Sector",  "Diamond"};
        ImGui::Combo("Type", &m_SelectedMesh, meshTypes, IM_ARRAYSIZE(meshTypes), IM_ARRAYSIZE(meshTypes));
        ImGui::SameLine();
        if (ImGui::Button("Assign")) {
            Rendering::MeshData data;
            switch (m_SelectedMesh) {
                case 0:
                    data = Rendering::MeshFactory::CreateQuad();
                    break;
                case 1:
                    data = Rendering::MeshFactory::CreatePointTopHex();
                    break;
                case 2:
                    data = Rendering::MeshFactory::CreateEquiTriangle(0.5f);
                    break;
                case 3:
                    data = Rendering::MeshFactory::CreateEllipse();
                    break;
                case 4:
                    data = Rendering::MeshFactory::CreateEllipse();
                    break;
                case 5:
                    data = Rendering::MeshFactory::CreateRegularPolygon(5);
                    break;
                case 6:
                    data = Rendering::MeshFactory::CreateRegularPolygon(6);
                    break;
                case 7:
                    data = Rendering::MeshFactory::CreateRegularPolygon(8);
                    break;
                case 8:
                    data = Rendering::MeshFactory::CreateRing();
                    break;
                case 9:
                    data = Rendering::MeshFactory::CreateSector();
                    break;
                case 10:
                    data = Rendering::MeshFactory::CreateDiamond();
                    break;
            }
            auto mesh = std::make_shared<Rendering::Mesh>(std::move(data));
            mesh->SetFactoryId(meshTypes[m_SelectedMesh]);
            Rendering::Renderer::SubmitInitTask([mesh] { mesh->InitGL(); });
            comp->mesh = std::move(mesh);
            MarkSceneChanged(engineContext);
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

            ImGui::Text("%s", comp->material->texture ? comp->material->texture->GetPath().c_str() : "None");
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                if (engineContext) {
                    const auto picked = FileDialogs::OpenFile(
                            *engineContext, {.filterName = "Image", .filterExt = "png,jpg,jpeg,bmp,tga"});
                    if (picked.has_value()) {
                        auto finalPath = ResolveOrImportPath(picked.value());
                        if (finalPath.has_value()) {
                            auto tex = std::make_shared<Rendering::Texture>(finalPath.value());
                            Rendering::Renderer::SubmitInitTask([tex] { tex->InitGL(); });
                            comp->material->texture = std::move(tex);
                            MarkSceneChanged(engineContext);
                        }
                    }
                }
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Shader");

            ImGui::Text("Vert: %s", comp->material->shader ? comp->material->shader->GetVertexPath().c_str() : "None");
            ImGui::SameLine();
            if (ImGui::Button("Vert##Shader")) {
                if (engineContext) {
                    const auto picked = FileDialogs::OpenFile(
                            *engineContext, {.filterName = "Vertex Shader", .filterExt = "vert,glsl"});
                    if (picked.has_value()) {
                        const auto finalPath = ResolveOrImportPath(picked.value());
                        if (finalPath.has_value() && comp->material->shader) {
                            comp->material->shader->GetVertexPath() = finalPath.value();
                            MarkSceneChanged(engineContext);
                        }
                    }
                }
            }

            ImGui::Text("Frag: %s",
                        comp->material->shader ? comp->material->shader->GetFragmentPath().c_str() : "None");
            ImGui::SameLine();
            if (ImGui::Button("Frag##Shader")) {
                if (engineContext) {
                    const auto picked = FileDialogs::OpenFile(
                            *engineContext, {.filterName = "Fragment Shader", .filterExt = "frag,glsl"});
                    if (picked.has_value()) {
                        const auto finalPath = ResolveOrImportPath(picked.value());
                        if (finalPath.has_value() && comp->material->shader) {
                            comp->material->shader->GetFragmentPath() = finalPath.value();
                            MarkSceneChanged(engineContext);
                        }
                    }
                }
            }

            ImGui::Spacing();
            if (ImGui::Button("Compile Shaders")) {
                if (comp->material->shader) {
                    auto shader = std::make_shared<Rendering::Shader>(comp->material->shader->GetVertexPath(),
                                                                      comp->material->shader->GetFragmentPath());
                    Rendering::Renderer::SubmitInitTask([shader] { shader->InitGL(); });
                    comp->material->shader = std::move(shader);
                }
            }
        } else {
            ImGui::TextDisabled("No material assigned");
            if (ImGui::Button("Create Material")) {
                comp->material = std::make_shared<Rendering::Material>();
                MarkSceneChanged(engineContext);
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

        for (int i = 0; i < 6; i++) {
            ImGui::PushID(i);
            ImGui::Text("Dir %d: %s", i, comp->textures[i] ? comp->textures[i]->GetPath().c_str() : "None");
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                if (engineContext) {
                    auto picked = FileDialogs::OpenFile(*engineContext,
                                                        {.filterName = "Image", .filterExt = "png,jpg,jpeg,bmp,tga"});
                    if (picked.has_value()) {
                        auto finalPath = ResolveOrImportPath(picked.value());
                        if (finalPath.has_value()) {
                            auto tex = std::make_shared<Rendering::Texture>(finalPath.value());
                            Rendering::Renderer::SubmitInitTask([tex] { tex->InitGL(); });
                            comp->textures[i] = std::move(tex);
                            MarkSceneChanged(engineContext);
                        }
                    }
                }
            }
            ImGui::PopID();
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

const char *Editor::UI::MapWidget::GetName() const { return "Map"; }

void Editor::UI::MapWidget::Draw(const ECS::Entity entity, Core::EngineContext *engineContext) {
    if (!entity.HasComponent<ECS::Components::MapComponent>())
        return;
    if (ImGui::CollapsingHeader(GetName())) {
        auto *comp = entity.GetComponent<ECS::Components::MapComponent>();
        char buffer[256];
        strncpy(buffer, comp->mapFilePath.c_str(), sizeof(buffer));
        if (ImGui::InputText("File Path", buffer, sizeof(buffer))) {
            comp->mapFilePath = buffer;
            MarkSceneChanged(engineContext);
        }
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

        // add Script button
        if (ImGui::Button("Add Script")) {
            if (engineContext) {
                const auto scriptPath =
                        FileDialogs::OpenFile(*engineContext, {.filterName = "Script Files", .filterExt = "obsl,txt"});
                if (scriptPath.has_value()) {
                    // resolve to vfs
                    const auto finalPath = ResolveOrImportPath(scriptPath.value(), "scripts");
                    if (finalPath.has_value()) {
                        // ensure component exists
                        if (!entity.HasComponent<ECS::Components::ScriptComponent>()) {
                            entity.AddComponent<ECS::Components::ScriptComponent>();
                            comp = entity.GetComponent<ECS::Components::ScriptComponent>();
                        }
                        // add the script path
                        comp->scriptPaths.push_back(finalPath.value());
                        comp->instance_envs.emplace_back();
                        comp->on_update_functions.emplace_back();
                        comp->on_destroy_functions.emplace_back();
                        comp->on_exit_functions.emplace_back();
                        comp->isInitialized.push_back(false);
                        comp->source_codes.emplace_back();
                        comp->ast_nodes.emplace_back();
                        comp->lastModified.push_back(std::filesystem::file_time_type::min());
                        MarkSceneChanged(engineContext);
                    }
                }
            }
        }
        // total count
        ImGui::SameLine();
        ImGui::TextDisabled("Total: %zu script(s)", comp->scriptPaths.size());
    }
}

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
