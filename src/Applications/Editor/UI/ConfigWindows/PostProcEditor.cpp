#include "PostProcEditor.h"

#include "Applications/Editor/Commands/EditorCommands.h"
#include "Core/Utils/ContainerUtils.h"
#include "Rendering/Renderer.h"
#include <imgui.h>

// todo: uniforms and shi

namespace Editor::UI {
    namespace {
        void DrawFXEntry(std::vector<Rendering::PostProcessing::PostEffect> &fxVec, const std::size_t index) {
            auto &fx = fxVec[index];
            ImGui::PushID(fx.shaderKey.c_str());

            ImGui::SetNextItemAllowOverlap();

            // render selectable
            ImGui::Selectable(fx.shaderKey.c_str(), false, 0, ImVec2(200.0f, 0.0f));

            // tetup drag source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                ImGui::SetDragDropPayload("POST_PROC_FX", &index, sizeof(std::size_t));
                ImGui::Text("Moving %s", fx.shaderKey.c_str());
                ImGui::EndDragDropSource();
            }

            // setup drag  target
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("POST_PROC_FX")) {
                    IM_ASSERT(payload->DataSize == sizeof(std::size_t));
                    const std::size_t payloadIndex = *static_cast<const std::size_t *>(payload->Data);

                    Core::Utils::vector::moveItem(fxVec, payloadIndex, index);
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::SameLine();
            ImGui::Checkbox("Enabled", &fx.enabled);

            ImGui::PopID();
        }
    } // namespace

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

        if (ImGui::Button("Save & Close")) {
            SaveConfig();
            isOpen = false;
        }

        if (ImGui::Button("Close")) {
            isOpen = false;
        }

        ImGui::End();
    }

    void PostProcEditor::Reload() {
        m_fxVecBackup = m_Context->renderer->GetPostProcessor().Effects();
        m_StagingVec = m_fxVecBackup;
    }

    void PostProcEditor::SaveConfig() { m_Undomgr->Execute(std::make_unique<Commands::PostProcUpdateCommand>(m_fxVecBackup, m_StagingVec), *m_Context); }

} // namespace Editor::UI
