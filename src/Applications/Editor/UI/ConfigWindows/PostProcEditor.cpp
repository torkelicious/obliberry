#include "PostProcEditor.h"

#include "Applications/Editor/Commands/EditorCommands.h"
#include "Core/Utils/ContainerUtils.h"
#include "Rendering/Renderer.h"
#include <imgui.h>

// todo: finish ts bs

namespace Editor::UI {

    void PostProcEditor::DrawFXEntry(std::vector<Rendering::PostProcessing::PostEffect> &fxVec, const std::size_t index) {
        auto &fx = fxVec[index];
        ImGui::PushID(fx.shaderKey.c_str());

        ImGui::SetNextItemAllowOverlap();

        // render selectable
        ImGui::Selectable(fx.shaderKey.c_str(), false, 0, ImVec2(200.0f, 0.0f));

        // tetup drag source
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("POST_PROC_FX", &index, sizeof(std::size_t));
            ImGui::EndDragDropSource();
        }

        // setup drag  target
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("POST_PROC_FX")) {
                IM_ASSERT(payload->DataSize == sizeof(std::size_t));
                if (const std::size_t payloadIndex = *static_cast<const std::size_t *>(payload->Data); payloadIndex != index) {
                    Core::Utils::vector::moveItem(fxVec, payloadIndex, index);
                    m_Dirty = true;
                }
            }

            ImGui::EndDragDropTarget();
        }


        ImGui::SameLine();
        ImGui::Checkbox("Enabled", &fx.enabled);

        if (!fx.uniforms.empty()) {
            if (ImGui::CollapsingHeader("Uniforms")) {
                UniformMapToImGui(fx.uniforms);
            }
        }

        ImGui::PopID();
    }

    void PostProcEditor::UniformMapToImGui(std::unordered_map<std::string, Rendering::PostProcessing::UniformValue> &uniforms) {
        if (uniforms.empty())
            return;

        for (auto &[key, value] : uniforms) {
            std::visit(
                    [&]<typename T0>(T0 &val) {
                        using T = std::decay_t<T0>;
                        if constexpr (std::is_same_v<T, float>) {
                            if (ImGui::DragFloat(key.c_str(), &val))
                                m_Dirty = true;
                        } else if constexpr (std::is_same_v<T, int>) {
                            if (ImGui::DragInt(key.c_str(), &val))
                                m_Dirty = true;
                        } else if constexpr (std::is_same_v<T, glm::vec2>) {
                            if (ImGui::DragFloat2(key.c_str(), &val.x))
                                m_Dirty = true;
                        } else if constexpr (std::is_same_v<T, glm::vec3>) {
                            if (ImGui::DragFloat3(key.c_str(), &val.x))
                                m_Dirty = true;
                        } else if constexpr (std::is_same_v<T, glm::vec4>) {
                            if (ImGui::DragFloat4(key.c_str(), &val.x))
                                m_Dirty = true;
                        }
                    },
                    value);
        }
    }

    void PostProcEditor::OnImGuiRender(bool &isOpen) {
        if (!isOpen || !m_Context->renderer)
            return;

        ImGui::Begin("Post Processing");

        ImGui::BeginChild("Effects");

        for (std::size_t i = 0; i < m_StagingVec.size(); ++i) {
            DrawFXEntry(m_StagingVec, i);
        }

        ImGui::EndChild();

        if (ImGui::Button("Apply")) {
            SaveConfig();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save & Close")) {
            SaveConfig();
            isOpen = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            isOpen = false;
        }

        ImGui::End();
    }

    // apply inplace without going through undomgr
    void PostProcEditor::Apply() {
        m_Context->renderer->GetPostProcessor().Effects() = m_StagingVec;
        m_Dirty = true;
    }

    // restore without going through undomgr
    void PostProcEditor::Restore() {
        m_Context->renderer->GetPostProcessor().Effects() = m_fxVecBackup;
        m_Dirty = false;
    }

    void PostProcEditor::Reload() {
        m_fxVecBackup = m_Context->renderer->GetPostProcessor().Effects();
        m_StagingVec = m_fxVecBackup;
        m_Dirty = false;
    }

    void PostProcEditor::SaveConfig() {
        m_Undomgr->Execute(std::make_unique<Commands::PostProcUpdateCommand>(m_fxVecBackup, m_StagingVec), *m_Context);
        m_Dirty = false;
    }

} // namespace Editor::UI
