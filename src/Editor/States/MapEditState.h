#pragma once
#include "../EditorState.h"

namespace Editor {
    // Placeholder
    class MapEditState : public EditorState {
    public:
        void OnUpdate(float dt) override;

        void OnHandleInput(float dt) override;

        void OnDrawPanels() override;
    };
} // namespace Editor
