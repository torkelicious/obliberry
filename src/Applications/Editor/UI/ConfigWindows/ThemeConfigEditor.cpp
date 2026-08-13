#include "ThemeConfigEditor.h"

#include "Applications/Editor/Commands/EditorCommands.h"
#include <cstdio>

namespace Editor::UI {

    void ThemeConfigEditor::Init(EditorContext &eCtx) { m_eCtx = &eCtx; }

    void ThemeConfigEditor::OnImGuiRender(bool &isOpen) {
        if (!m_eCtx || !isOpen)
            return;

        ImGui::Begin("Editor Theme Editor");

        char nameBuf[128]{};
        std::snprintf(nameBuf, sizeof(nameBuf), "%s", m_LocalTheme.name.c_str());
        if (ImGui::InputText("Theme Name", nameBuf, sizeof(nameBuf))) {
            m_LocalTheme.name = nameBuf;
        }

        // semantic palette
        if (ImGui::CollapsingHeader("Color Palette", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushID("SemanticPaletteScope");

            bool paletteChanged = false;

            ImGui::TextDisabled("Base");
            paletteChanged |= ImGui::ColorEdit4("Background", &m_Palette.bg.x);
            paletteChanged |= ImGui::ColorEdit4("Background Alt", &m_Palette.bgAlt.x);
            paletteChanged |= ImGui::ColorEdit4("Background Active", &m_Palette.bgActive.x);

            ImGui::Spacing();
            ImGui::TextDisabled("Accents");
            paletteChanged |= ImGui::ColorEdit4("Accent", &m_Palette.accent.x);
            paletteChanged |= ImGui::ColorEdit4("Accent Hover", &m_Palette.accentHover.x);
            paletteChanged |= ImGui::ColorEdit4("Accent Active", &m_Palette.accentActive.x);

            ImGui::Spacing();
            ImGui::TextDisabled("Typography / Borders");
            paletteChanged |= ImGui::ColorEdit4("Text", &m_Palette.text.x);
            paletteChanged |= ImGui::ColorEdit4("Text Muted", &m_Palette.textDim.x);
            paletteChanged |= ImGui::ColorEdit4("Border", &m_Palette.border.x);

            if (paletteChanged) {
                ApplyPalette();
            }

            ImGui::PopID();
        }

        // optional overrides
        if (ImGui::CollapsingHeader("Advanced Color Overrides")) {
            ImGui::PushID("AdvancedColorsScope");

            for (const auto &[id, name] : Theme::kColorTable) {
                ImGui::PushID(id);

                ImVec4 value = m_LocalTheme.GetColor(id).value_or(ImGui::GetStyle().Colors[id]);
                if (ImGui::ColorEdit4(name.data(), &value.x)) {
                    m_LocalTheme.SetColor(id, value);
                    Apply(m_LocalTheme);
                }

                ImGui::PopID();
            }

            ImGui::PopID();
        }

        // layout & spacing
        if (ImGui::CollapsingHeader("Layout & Geometry")) {
            ImGui::PushID("GeometryScope");

            for (const auto &info : Theme::kStyleVarTable) {
                ImGui::PushID(info.id);

                std::visit(
                        [&](auto memberPtr) {
                            using MemberT = std::decay_t<decltype(memberPtr)>;
                            if constexpr (std::is_same_v<MemberT, Theme::FloatMember>) {
                                float value = m_LocalTheme.GetFloat(info.id).value_or(ImGui::GetStyle().*memberPtr);
                                if (ImGui::DragFloat(info.name.data(), &value, 0.1f, 0.0f, 32.0f)) {
                                    m_LocalTheme.SetFloat(info.id, value);
                                    Apply(m_LocalTheme);
                                }
                            } else {
                                ImVec2 value = m_LocalTheme.GetVec2(info.id).value_or(ImGui::GetStyle().*memberPtr);
                                if (ImGui::DragFloat2(info.name.data(), &value.x, 0.1f, 0.0f, 64.0f)) {
                                    m_LocalTheme.SetVec2(info.id, value);
                                    Apply(m_LocalTheme);
                                }
                            }
                        },
                        info.member);

                ImGui::PopID();
            }

            ImGui::PopID();
        }

        ImGui::Separator();

        if (ImGui::Button("Save")) {
            SaveConfig();
            isOpen = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Close & Discard changes")) {
            m_eCtx->theme = m_OldTheme;
            Theme::Apply(m_eCtx->theme);
            isOpen = false;
        }

        ImGui::End();
    }

    void ThemeConfigEditor::Reload() {
        m_OldTheme = m_eCtx->theme;
        m_LocalTheme = m_OldTheme;

        m_Palette.bg = m_LocalTheme.GetColor(ImGuiCol_WindowBg).value_or(m_Palette.bg);
        m_Palette.bgAlt = m_LocalTheme.GetColor(ImGuiCol_MenuBarBg).value_or(m_Palette.bgAlt);
        m_Palette.bgActive = m_LocalTheme.GetColor(ImGuiCol_FrameBgActive).value_or(m_Palette.bgActive);
        m_Palette.accent = m_LocalTheme.GetColor(ImGuiCol_Button).value_or(m_Palette.accent);
        m_Palette.accentHover = m_LocalTheme.GetColor(ImGuiCol_ButtonHovered).value_or(m_Palette.accentHover);
        m_Palette.accentActive = m_LocalTheme.GetColor(ImGuiCol_ButtonActive).value_or(m_Palette.accentActive);
        m_Palette.text = m_LocalTheme.GetColor(ImGuiCol_Text).value_or(m_Palette.text);
        m_Palette.textDim = m_LocalTheme.GetColor(ImGuiCol_TextDisabled).value_or(m_Palette.textDim);
        m_Palette.border = m_LocalTheme.GetColor(ImGuiCol_Border).value_or(m_Palette.border);
    }

    void ThemeConfigEditor::ApplyPalette() {
        const Theme::Theme generated = Theme::BuildFromPalette(m_Palette, m_LocalTheme.name);
        m_LocalTheme.colors = generated.colors;
        Theme::Apply(m_LocalTheme);
    }

    void ThemeConfigEditor::SaveConfig() { m_Undomgr->Execute(std::make_unique<Commands::ThemeUpdateCommand>(m_OldTheme, m_LocalTheme, *m_eCtx), *m_Context); }

} // namespace Editor::UI
