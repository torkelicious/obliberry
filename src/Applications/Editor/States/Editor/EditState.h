#pragma once
#include "Applications/Editor/States/EditorStateBase.h"
#include "Rendering/Types/Transform.h"
#include "UI/UIGizmo.h"
#include <glm/glm.hpp>
#include "imgui.h"
#include "ImGuizmo.h"
#include "Applications/Editor/UI/Panels/ProjectBrowserPanel.h"
#include "Applications/Editor/UI/Panels/Editor/MeshCreatorPanel.h"

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

        static void ShowMeshCreator() { s_ShowMeshCreator = true; }
        static void HideMeshCreator() { s_ShowMeshCreator = false; }
        static bool IsMeshCreatorOpen() { return s_ShowMeshCreator; }

    private:
        void Entity_DrawGizmoForSelected() const; // entities
        void EntityGizmoTranslate(Rendering::Transform &localTransform, Rendering::Transform &worldTransform, bool isBillboard);

        void UI_DrawGizmoForSelected() const; // UI system
        void UI_HandleGizmoInput();
        [[nodiscard]] glm::vec2 GetUIGizmoMousePos() const;

        bool m_HideGameUI = false;

        static ImGuizmo::OPERATION mCurrentGizmoOperation;
        static ImGuizmo::MODE mCurrentGizmoMode;

        bool m_GizmoDragging = false;
        glm::vec3 m_GizmoStartPos{0.0f};
        glm::vec3 m_GizmoStartRot{0.0f};
        glm::vec3 m_GizmoStartScale{1.0f};

        // UI gizmo dragging
        ::UI::HandleType m_UIDragHandle = ::UI::HandleType::None;
        glm::vec2 m_UIDragStartMouse{0.0f};
        glm::vec2 m_UIDragStartWorldPos{0.0f};
        glm::vec2 m_UIDragStartScale{0.0f};
        bool m_UIDragStarted = false; // dead-zone check
        ::UI::HandleType m_UIHoveredHandle = ::UI::HandleType::None;

        // mesh panel
        UI::MeshCreatorPanel m_MeshCreatorPanel;
        inline static bool s_ShowMeshCreator = false;
    };
} // namespace Editor::States
