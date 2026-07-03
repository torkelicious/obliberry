#pragma once
#include "../EditorState.h"

namespace Editor {
    class PlayState : public EditorState {
    public:
        void OnEnter(EditorLayer &editor) override;

        void OnExit(EditorLayer &editor) override;

        void OnUpdate(EditorLayer &editor, float dt) override;

        void OnHandleInput(EditorLayer &editor, float dt) override;

        void OnDrawPanels(EditorLayer &editor) override;

        bool CanSaveScene() const override { return false; }
        bool CanSaveSceneAs() const override { return false; }
        bool IsPlayMode() const override { return true; }
        const char *PlayStopLabel() const override { return "Stop"; }
    };
} // namespace Editor
