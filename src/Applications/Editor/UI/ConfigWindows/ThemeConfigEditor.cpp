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
        if (ImGui::CollapsingHeader("Color Palette")) {
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

        if (ImGui::CollapsingHeader("Fonts")) {
            ImGui::PushID("FontScope");

            static int selectedFontIdx = -1;
            static char fontNameBuffer[128] = {};
            static int nameBufferFontIdx = -1;

            auto &fonts = m_LocalFontSet.fonts;

            ImGui::BeginChild("Fontset", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);

            if (fonts.empty()) {
                ImGui::TextDisabled("No fonts loaded");
                selectedFontIdx = -1;
                nameBufferFontIdx = -1;
            } else {
                if (selectedFontIdx >= static_cast<int>(fonts.size())) {
                    selectedFontIdx = static_cast<int>(fonts.size()) - 1;
                }

                for (int i = 0; i < static_cast<int>(fonts.size()); ++i) {
                    ImGui::PushID(i);
                    ImGui::PushFont(fonts[i].fontPtr);
                    if (const bool selected = (selectedFontIdx == i); ImGui::Selectable(fonts[i].name.c_str(), selected)) {
                        selectedFontIdx = i;
                        std::snprintf(fontNameBuffer, sizeof(fontNameBuffer), "%s", fonts[i].name.c_str());
                        nameBufferFontIdx = i;
                    }
                    ImGui::PopFont();
                    ImGui::PopID();
                }
            }

            ImGui::EndChild();

            // font options
            if (selectedFontIdx >= 0 && selectedFontIdx < static_cast<int>(fonts.size())) {

                auto &selectedFont = fonts[selectedFontIdx];

                if (nameBufferFontIdx != selectedFontIdx) {
                    std::snprintf(fontNameBuffer, sizeof(fontNameBuffer), "%s", selectedFont.name.c_str());

                    nameBufferFontIdx = selectedFontIdx;
                }

                ImGui::BeginChild("FontOptions", ImVec2(0, 0), ImGuiChildFlags_Borders);

                ImGui::InputText("Name", fontNameBuffer, IM_ARRAYSIZE(fontNameBuffer));

                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    selectedFont.name = fontNameBuffer;
                }

                ImGui::Separator();

                ImGui::DragFloat("Font size (px)", &selectedFont.sizePixels, 0.1f, 1.0f, 200.0f, "%.1f");

                ImGui::Separator();

                ImGui::Checkbox("Merge into previous font", &selectedFont.mergeIntoPrevious);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Adds this font as a fallback to the previous font in the list.\nUseful for icon fonts or extending glyph coverage.");
                }

                if (selectedFont.mergeIntoPrevious) {
                    ImGui::DragFloat("Icon min advance X", &selectedFont.iconMinAdvanceX, 0.1f, 0.0f, 100.0f, "%.1f");
                }

                ImGui::Separator();

                const int currentRole = static_cast<int>(selectedFont.role);
                if (const char *fontRoles[] = {"Body", "Bold", "Monospace", "Small", "Heading", "Icon"}; ImGui::BeginCombo("Role", fontRoles[currentRole])) {
                    for (int i = 0; i < IM_ARRAYSIZE(fontRoles); ++i) {
                        const auto role = static_cast<Theme::FontRole>(i);
                        const bool selected = (selectedFont.role == role);

                        if (const bool taken = IsFontRoleTaken(fonts, role, selectedFontIdx); ImGui::Selectable(fontRoles[i], selected, taken && !selected ? ImGuiSelectableFlags_Disabled : 0)) {
                            if (taken && !selected) {
                                // swap roles
                                for (auto &otherFont : fonts) {
                                    if (&otherFont != &selectedFont && otherFont.role == role) {
                                        otherFont.role = selectedFont.role;
                                        break;
                                    }
                                }
                            }
                            selectedFont.role = role;
                        }

                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }

                    ImGui::EndCombo();
                }
                ImGui::EndChild();
            }


            if (ImGui::Button("Import new UI font")) {
                fonts.push_back(FontFromDialog());

                selectedFontIdx = static_cast<int>(fonts.size()) - 1;

                std::snprintf(fontNameBuffer, sizeof(fontNameBuffer), "%s", fonts.back().name.c_str());
                nameBufferFontIdx = selectedFontIdx;
                if (IsFontRoleTaken(fonts, fonts[selectedFontIdx].role, selectedFontIdx)) {
                    ImGui::TextColored({1.0f, 0.0f, 0.0f, 1.0f}, "Imported font has conflicting role!"); // todo: improve this msg
                }
            }

            if (selectedFontIdx >= 0 && selectedFontIdx < static_cast<int>(fonts.size())) {
                ImGui::SameLine();
                if (ImGui::Button("Delete")) {
                    fonts.erase(fonts.begin() + selectedFontIdx);
                    if (fonts.empty()) {
                        selectedFontIdx = -1;
                    } else if (selectedFontIdx >= static_cast<int>(fonts.size())) {
                        selectedFontIdx = static_cast<int>(fonts.size()) - 1;
                    }
                }
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

        m_LocalFontSet = m_eCtx->fontset;
        m_OldFontSet = m_LocalFontSet;

        for (auto &font : m_LocalFontSet.fonts) {
            font.fontPtr = nullptr;
        }
        for (auto &font : m_OldFontSet.fonts) {
            font.fontPtr = nullptr;
        }

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

    void ThemeConfigEditor::SaveConfig() { m_Undomgr->Execute(std::make_unique<Commands::ThemeUpdateCommand>(m_OldTheme, m_LocalTheme, m_OldFontSet, m_LocalFontSet, *m_eCtx), *m_Context); }

} // namespace Editor::UI
