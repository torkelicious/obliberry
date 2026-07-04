#pragma once
#include "../EditorState.h"

#include "imgui.h"
#include "ImGuizmo.h"

namespace Editor {
    class EditState : public EditorState {
    public:
        enum OPERATION : uint8_t {
            TRANSLATE,
            ROTATE,
            SCALE
        };

        // enum MODE : uint8_t {
        //     LOCAL,
        //     WORLD
        // };

        void OnUpdate(float dt) override;

        void OnHandleInput(float dt) override;

        void OnDrawPanels() override;

    private:
        void DrawGizmoForSelected();
        void EditTransform(Rendering::Transform &transform);
        static ImGuizmo::OPERATION mCurrentGizmoOperation;
        static ImGuizmo::MODE mCurrentGizmoMode;
    };
} // namespace Editor
