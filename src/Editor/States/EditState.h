#pragma once
#include "../EditorState.h"

namespace Editor {
    class EditState : public EditorState {
    public:
        void OnUpdate(EditorLayer &editor, float dt) override;

        void OnHandleInput(EditorLayer &editor, float dt) override;

        void OnDrawPanels(EditorLayer &editor) override;
    };
} // namespace Editor
