#pragma once
#include "UI/Themeing/EditorTheme.h"
#include <atomic>

// same idea as enginecontext, but for editor-only stuff....
struct EditorContext {
    Editor::UI::Theme::Theme theme = Editor::UI::Theme::DefaultDarkTheme();
    Editor::UI::Theme::FontSet fontset = Editor::UI::Theme::DefaultFontSet();
    std::atomic<bool> fontsDirty{false};
};
