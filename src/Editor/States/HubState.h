#pragma once
#include "../EditorState.h"

namespace Editor {

    class HubState : public EditorState {
    public:
        void OnEnter() override;
        void OnExit() override;
        void OnUpdate(float dt) override;
        void OnHandleInput(float dt) override;
        void OnDrawPanels() override;
        void OnRender() override;

        bool CanSaveScene() const override { return false; }
        bool CanSaveSceneAs() const override { return false; }
    };

} // namespace Editor
