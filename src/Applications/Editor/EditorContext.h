#pragma once
#include "Applications/Editor/Clipboard.h"
#include "Applications/Editor/UI/Themeing/Builtins.h"
#include <atomic>

// same idea as enginecontext, but for editor-only stuff....
struct EditorContext {
    Editor::UI::Theme::Theme theme = Editor::UI::Theme::DefaultDarkTheme();
    Editor::UI::Theme::FontSet fontset = Editor::UI::Theme::DefaultFontSet();
    std::atomic<bool> *fontsDirty = nullptr;
    Editor::Clipboard *clipboard = nullptr;
};
