#pragma once
#include "Applications/Editor/States/EditorStateBase.h"
#include "Rendering/Transform.h"
#include <glm/glm.hpp>
#include "imgui.h"
#include "ImGuizmo.h"

namespace Editor::States {
    class EditState : public EditorStateBase {
    public:
        static void SetGizmoOperation(const ImGuizmo::OPERATION op) { mCurrentGizmoOperation = op; }
        static ImGuizmo::OPERATION GetGizmoOperation() { return mCurrentGizmoOperation; }

        void OnEnter() override;

        void OnUpdate(float dt) override;

        void OnHandleInput(float dt) override;

        void OnDrawPanels() override;

        void OnRender() override;

        void OnDrawModeToolbar() override;

        void OnSaveKey() override;

    private:
        void DrawGizmoForSelected() const;
        void EditTransform(Rendering::Transform &localTransform, Rendering::Transform &worldTransform, bool isBillboard);

        bool m_HideGameUI = false;

        static ImGuizmo::OPERATION mCurrentGizmoOperation;
        static ImGuizmo::MODE mCurrentGizmoMode;

        bool m_GizmoDragging = false;
        glm::vec3 m_GizmoStartPos{0.0f};
        glm::vec3 m_GizmoStartRot{0.0f};
        glm::vec3 m_GizmoStartScale{1.0f};
    };
} // namespace Editor::States
