#pragma once
#include "Applications/Editor/States/EditorStateBase.h"
#include <string>

namespace Editor::States {
    class PlayState : public EditorStateBase {
    public:
        void OnEnter() override;

        void OnExit() override;

        void OnUpdate(float dt) override;

        void OnHandleInput(float dt) override;

        void OnDrawPanels() override;

        void OnRender() override;

        [[nodiscard]] bool CanSaveScene() const override { return false; }
        [[nodiscard]] bool CanSaveSceneAs() const override { return false; }
        [[nodiscard]] bool IsPlayMode() const override { return true; }
        [[nodiscard]] const char *PlayStopLabel() const override { return "Stop"; }
        [[nodiscard]] bool ShouldDrawProjectBrowser() const override { return false; }

    private:
        std::string m_EntryScenePath;
    };
} // namespace Editor::States
