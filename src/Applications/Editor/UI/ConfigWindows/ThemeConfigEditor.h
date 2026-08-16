#pragma once
#include "ConfigWindow.h"
#include "Applications/Editor/EditorContext.h"
#include "Applications/Editor/UI/Themeing/EditorTheme.h"
#include "Applications/Editor/Platform/FileDialogs.h"

namespace Editor::UI {
    class ThemeConfigEditor : public ConfigEditor {
    public:
        void Init(EditorContext &eCtx);
        void OnImGuiRender(bool &isOpen) override;
        void Reload() override;
        void SaveConfig() override;

    private:
        void ApplyPalette();

        static bool IsFontRoleTaken(const std::vector<Theme::FontConfig> &fonts, const Theme::FontRole role, const int exceptIdx) {
            for (int i = 0; i < static_cast<int>(fonts.size()); ++i) {
                if (i != exceptIdx && fonts[i].role == role) {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] Theme::FontConfig FontFromDialog() const {
            Theme::FontConfig font;
            constexpr Platform::FileDialogOptions opts{
                    .filterName = "Fonts",
                    .filterExt = "ttf,otf",
                    .title = "Pick font",
                    .acceptBtnLabel = "Select",
            };
            if (const auto file = Platform::FileDialogs::OpenFile(*m_Context, opts); file.has_value()) {
                font = Theme::LoadFontConfig(file.value());
            }
            return font;
        }


        EditorContext *m_eCtx = nullptr;
        Theme::Theme m_LocalTheme;
        Theme::Theme m_OldTheme;
        Theme::SemanticPalette m_Palette;

        Theme::FontSet m_LocalFontSet;
        Theme::FontSet m_OldFontSet;

        int m_SelectedFontIdx = -1;
        char m_FontNameBuffer[128] = {};
        int m_NameBufferFontIdx = -1;
        char m_ColorSearchBuffer[128] = {};
        char m_LayoutSearchBuffer[128] = {};
    };
} // namespace Editor::UI
