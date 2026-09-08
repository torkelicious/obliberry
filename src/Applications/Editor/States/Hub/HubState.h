#pragma once
#include "../EditorStateBase.h"
#include "Applications/Editor/States/Hub/ProjectHistory.h"
#include <optional>
#include <string>

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

    private:
        std::optional<size_t> DrawProjectEntry(size_t index, const ProjectHistoryEntry &entry) const;
        mutable std::string m_OpenProjectError;
    };

} // namespace Editor::States
