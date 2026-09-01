#include "PostProcEditor.h"

#include "Applications/Editor/Commands/EditorCommands.h"
#include "Core/Utils/ContainerUtils.h"
#include "Rendering/PostProcessing/InternalPostProcFx.h"
#include "Rendering/Renderer.h"
#include <algorithm>
#include <imgui.h>

namespace Editor::UI {

    namespace {

        // strip the "[Engine_PP] " prefix for display
        const char *ShortShaderName(const std::string &key) {
            static constexpr std::string_view kPrefix = "[Engine_PP] ";
            if (key.starts_with(kPrefix))
                return key.c_str() + kPrefix.size();
            return key.c_str();
        }

        // drag range follows the magnitude of the current val
        void ComputeDragRange(const float *values, const int count, float &lo, float &hi, float &speed) {
            lo = 0.0f;
            hi = 0.01f;
            for (int i = 0; i < count; ++i) {
                lo = std::min(lo, values[i] * 2.0f);
                hi = std::max(hi, std::abs(values[i]) * 2.0f);
            }
            speed = std::max((hi - lo) * 0.005f, 0.0001f);
        }

        bool DragFloatClamped(const char *label, float *v) {
            float lo, hi, speed;
            ComputeDragRange(v, 1, lo, hi, speed);
            return ImGui::DragFloat(label, v, speed, lo, hi, "%.4g");
        }

        bool DragFloatNClamped(const char *label, float *v, const int count) {
            float lo, hi, speed;
            ComputeDragRange(v, count, lo, hi, speed);
            switch (count) {
                case 2:
                    return ImGui::DragFloat2(label, v, speed, lo, hi, "%.4g");
                case 3:
                    return ImGui::DragFloat3(label, v, speed, lo, hi, "%.4g");
                default:
                    return ImGui::DragFloat4(label, v, speed, lo, hi, "%.4g");
            }
        }

        bool DragIntClamped(const char *label, int *v) {
            const int lo = std::min(0, *v * 2);
            const int hi = std::max({2, *v * 2, *v + 1});
            return ImGui::DragInt(label, v, 0.05f, lo, hi);
        }

    } // namespace

    void PostProcEditor::DrawEffectList() {
        const float lineHeight = ImGui::GetFrameHeightWithSpacing();
        const float listHeight = lineHeight * std::min<std::size_t>(m_StagingVec.size() + 1, 6) + ImGui::GetStyle().WindowPadding.y;

        ImGui::BeginChild("EffectChain", ImVec2(0, listHeight), ImGuiChildFlags_Borders);

        if (m_StagingVec.empty())
            ImGui::TextDisabled("No effects");

        for (std::size_t i = 0; i < m_StagingVec.size(); ++i) {
            auto &fx = m_StagingVec[i];
            ImGui::PushID(static_cast<int>(i));

            // row overlay
            ImGui::SetNextItemAllowOverlap();
            if (ImGui::Selectable("##fx", m_SelectedFx == static_cast<int>(i), 0, ImVec2(0.0f, 0.0f)))
                m_SelectedFx = static_cast<int>(i);

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                ImGui::SetDragDropPayload("POST_PROC_FX", &i, sizeof(std::size_t));
                ImGui::Text("%s", ShortShaderName(fx.shaderKey));
                ImGui::EndDragDropSource();
            }

            // drop target
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("POST_PROC_FX")) {
                    IM_ASSERT(payload->DataSize == sizeof(std::size_t));
                    // commit immediately
                    if (const std::size_t from = *static_cast<const std::size_t *>(payload->Data); from != i) {
                        Core::Utils::vector::moveItem(m_StagingVec, from, i);
                        m_SelectedFx = static_cast<int>(i);
                        NotifyInstantEdit();
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // draw the entry
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::BeginGroup();
            ImGui::Text("%zu.", i + 1);
            ImGui::SameLine();
            ImGui::TextUnformatted(ShortShaderName(fx.shaderKey));
            ImGui::EndGroup();

            // checkbox
            ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::GetFrameHeight());
            if (ImGui::Checkbox("##enabled", &fx.enabled))
                NotifyInstantEdit();

            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    void PostProcEditor::DrawEffectDetails(Rendering::PostProcessing::PostEffect &fx) {
        ImGui::BeginChild("EffectDetails", ImVec2(0, 0), ImGuiChildFlags_Borders);

        ImGui::Text("%s", fx.shaderKey.c_str());
        if (!fx.shader)
            ImGui::TextColored({1.0f, 0.3f, 0.3f, 1.0f}, "shader not resolved, effect is skipped");
        else if (!fx.enabled)
            ImGui::TextDisabled("(disabled)");

        ImGui::Separator();

        int passes = fx.passes;
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragInt("Passes", &passes, 0.1f, 1, 16)) {
            fx.passUniforms.resize(static_cast<std::size_t>(passes));
            fx.passes = passes;
            m_Dirty = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            m_CommitPending = true;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("How many times the effect runs, ping-ponging between targets.");

        // scene texture access
        if (ImGui::Checkbox("Uses scene texture (u_Scene)", &fx.wantsSceneTexture))
            NotifyInstantEdit();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Also binds the original scene texture on unit 1\nfor compositing effects like bloom.");

        ImGui::Separator();

        if (!fx.uniforms.empty()) {
            ImGui::SeparatorText("Uniforms");
            UniformMapToImGui(fx.uniforms);
        }

        ImGui::TextDisabled("ctrl+click a drag to type in an exact value");

        for (int pass = 0; pass < static_cast<int>(fx.passUniforms.size()); ++pass) {
            auto &bag = fx.passUniforms[pass];
            if (bag.empty())
                continue;

            ImGui::PushID(pass);
            ImGui::SeparatorText((std::string("Pass ") + std::to_string(pass + 1)).c_str());
            ImGui::Indent();
            UniformMapToImGui(bag);
            ImGui::Unindent();
            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    void PostProcEditor::UniformMapToImGui(std::unordered_map<std::string, Rendering::PostProcessing::UniformValue> &uniforms) {
        if (uniforms.empty())
            return;

        std::vector<std::pair<const std::string, Rendering::PostProcessing::UniformValue> *> entries;
        entries.reserve(uniforms.size());
        for (auto &entry : uniforms)
            entries.push_back(&entry);
        std::ranges::sort(entries, [](const auto *a, const auto *b) { return a->first < b->first; });

        for (auto *entry : entries) {
            const std::string &key = entry->first;
            auto &value = entry->second;

            std::visit(
                    [&]<typename T0>(T0 &val) {
                        using T = std::decay_t<T0>;

                        bool edited = false;
                        if constexpr (std::is_same_v<T, float>) {
                            edited = DragFloatClamped(key.c_str(), &val);
                        } else if constexpr (std::is_same_v<T, int>) {
                            edited = DragIntClamped(key.c_str(), &val);
                        } else if constexpr (std::is_same_v<T, glm::vec2>) {
                            edited = DragFloatNClamped(key.c_str(), &val.x, 2);
                        } else if constexpr (std::is_same_v<T, glm::vec3>) {
                            if (key.find("Color") != std::string::npos)
                                edited = ImGui::ColorEdit3(key.c_str(), &val.x);
                            else
                                edited = DragFloatNClamped(key.c_str(), &val.x, 3);
                        } else if constexpr (std::is_same_v<T, glm::vec4>) {
                            if (key.find("Color") != std::string::npos)
                                edited = ImGui::ColorEdit4(key.c_str(), &val.x);
                            else
                                edited = DragFloatNClamped(key.c_str(), &val.x, 4);
                        }

                        if (edited)
                            m_Dirty = true;

                        if (ImGui::IsItemDeactivatedAfterEdit())
                            m_CommitPending = true;
                    },
                    value);
        }
    }

    void PostProcEditor::OnImGuiRender(bool &isOpen) {
        if (!isOpen || !m_Context->renderer)
            return;

        ImGui::SetNextWindowSize(ImVec2(420.0f, 0.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Post Processing", &isOpen);

        DrawEffectList();

        if (m_SelectedFx >= static_cast<int>(m_StagingVec.size()))
            m_SelectedFx = static_cast<int>(m_StagingVec.size()) - 1;

        if (m_SelectedFx >= 0) {
            ImGui::Spacing();
            ImGui::PushID(m_SelectedFx);
            DrawEffectDetails(m_StagingVec[m_SelectedFx]);
            ImGui::PopID();
        }

        ImGui::Spacing();

        const float delW = ImGui::CalcTextSize("Delete").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - delW - ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::BeginCombo("##addfx", "Add effect...")) {
            namespace Builtins = Rendering::PostProcessing::Builtins;
            for (const auto &reg : Builtins::ppEffectRegistrations) {
                const std::string key = "[Engine_PP] " + std::string(reg.shaderName);
                if (ImGui::Selectable(reg.shaderName, false)) {
                    Rendering::PostProcessing::PostEffect fx;
                    fx.shaderKey = key;
                    fx.enabled = reg.enabled;
                    fx.passes = reg.passes;
                    fx.wantsSceneTexture = reg.wantsSceneTexture;
                    for (const auto &[name, value] : reg.uniforms)
                        fx.uniforms[name] = value;
                    for (const auto &passBag : reg.passUniforms) {
                        std::unordered_map<std::string, Rendering::PostProcessing::UniformValue> bag;
                        for (const auto &[name, value] : passBag)
                            bag[name] = value;
                        fx.passUniforms.push_back(std::move(bag));
                    }
                    fx.ResolveShader();

                    m_StagingVec.push_back(std::move(fx));
                    m_SelectedFx = static_cast<int>(m_StagingVec.size()) - 1;
                    NotifyInstantEdit();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();

        const bool canDelete = m_SelectedFx >= 0 && m_SelectedFx < static_cast<int>(m_StagingVec.size());
        if (!canDelete)
            ImGui::BeginDisabled();
        if (ImGui::Button("Delete", ImVec2(delW, 0.0f))) {
            m_StagingVec.erase(m_StagingVec.begin() + m_SelectedFx);
            if (m_StagingVec.empty())
                m_SelectedFx = -1;
            else if (m_SelectedFx >= static_cast<int>(m_StagingVec.size()))
                m_SelectedFx = static_cast<int>(m_StagingVec.size()) - 1;
            NotifyInstantEdit();
        }
        if (!canDelete)
            ImGui::EndDisabled();

        ImGui::Separator();

        if (m_CommitPending) {
            Commit();
        } else if (m_Dirty) {
            Apply();
        }

        if (ImGui::Button("Apply"))
            SaveConfig();
        ImGui::SameLine();
        if (ImGui::Button("Save & Close")) {
            SaveConfig();
            isOpen = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            Restore();
            m_StagingVec = m_fxVecBackup;
            isOpen = false;
        }
        ImGui::SameLine();
        if (m_Dirty)
            ImGui::TextDisabled("(previewing)");
        else
            ImGui::TextDisabled("(synced)");

        ImGui::End();

        if (!isOpen && m_Dirty) {
            Restore();
            m_StagingVec = m_fxVecBackup;
        }
    }

    // apply inplace without going through undomgr
    void PostProcEditor::Apply() { m_Context->renderer->GetPostProcessor().Effects() = m_StagingVec; }

    // restore without going through undomgr
    void PostProcEditor::Restore() {
        m_Context->renderer->GetPostProcessor().Effects() = m_fxVecBackup;
        m_Dirty = false;
    }

    void PostProcEditor::Reload() {
        m_fxVecBackup = m_Context->renderer->GetPostProcessor().Effects();
        m_StagingVec = m_fxVecBackup;
        if (m_SelectedFx >= static_cast<int>(m_StagingVec.size()))
            m_SelectedFx = static_cast<int>(m_StagingVec.size()) - 1;
        m_Dirty = false;
        m_CommitPending = false;
    }

    void PostProcEditor::SaveConfig() {
        if (!m_Dirty && !m_CommitPending)
            return;
        Commit();
    }

    void PostProcEditor::Commit() {
        m_Undomgr->Execute(std::make_unique<Commands::PostProcUpdateCommand>(m_fxVecBackup, m_StagingVec), *m_Context);
        m_fxVecBackup = m_StagingVec;
        m_Dirty = false;
        m_CommitPending = false;
    }

} // namespace Editor::UI
