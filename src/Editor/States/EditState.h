#pragma once
#include "../EditorState.h"
#include <cstdint>
#include "imgui.h"
#include "ImGuizmo.h"

namespace Editor {
    class EditState : public EditorState {
    public:
        static void SetGizmoOperation(const ImGuizmo::OPERATION op) { mCurrentGizmoOperation = op; }
        static ImGuizmo::OPERATION GetGizmoOperation() { return mCurrentGizmoOperation; }

        void OnEnter() override;

        void OnUpdate(float dt) override;

        void OnHandleInput(float dt) override;

        void OnDrawPanels() override;

        void OnRender() override;

        void OnDrawModeToolbar() override;

    private:
        void DrawGizmoForSelected() const;

        void EditTransform(Rendering::Transform &transform, bool isBillboard) const;

        static ImGuizmo::OPERATION mCurrentGizmoOperation;
        static ImGuizmo::MODE mCurrentGizmoMode;
    };
} // namespace Editor
