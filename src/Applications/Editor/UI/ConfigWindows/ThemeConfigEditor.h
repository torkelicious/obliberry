#pragma once
#include "ConfigWindow.h"
#include "Applications/Editor/EditorContext.h"
#include "Applications/Editor/UI/Themeing/EditorTheme.h"

namespace Editor::UI {
    class ThemeConfigEditor : public ConfigEditor {
    public:
        void Init(EditorContext &eCtx);
        void OnImGuiRender(bool &isOpen) override;
        void Reload() override;
        void SaveConfig() override;

    private:
        void ApplyPalette();

        EditorContext *m_eCtx = nullptr;
        Theme::Theme m_LocalTheme;
        Theme::Theme m_OldTheme;
        Theme::SemanticPalette m_Palette;
    };
} // namespace Editor::UI
