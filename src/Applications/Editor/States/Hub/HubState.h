#pragma once
#include "../EditorStateBase.h"

namespace Editor::States {

    class HubState : public EditorStateBase {
    public:
        void OnEnter() override;
        void OnExit() override;
        void OnUpdate(float dt) override;
        void OnHandleInput(float dt) override;
        void OnDrawPanels() override;
        void OnRender() override;

        bool CanSaveScene() const override { return false; }
        bool CanSaveSceneAs() const override { return false; }
        bool ShouldDrawProjectBrowser() const override { return false; }
    };

} // namespace Editor::States
