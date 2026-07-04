#pragma once
#include "../EditorState.h"

namespace Editor {
    class PlayState : public EditorState {
    public:
        void OnEnter() override;

        void OnExit() override;

        void OnUpdate(float dt) override;

        void OnHandleInput(float dt) override;

        void OnDrawPanels() override;

        bool CanSaveScene() const override { return false; }
        bool CanSaveSceneAs() const override { return false; }
        bool IsPlayMode() const override { return true; }
        const char *PlayStopLabel() const override { return "Stop"; }
    };
} // namespace Editor
