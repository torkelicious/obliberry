#pragma once

#include "EditorPanel.h"

#include <imgui.h>
#include <ImGuizmo.h>

#include "Rendering/Types/Camera.h"

namespace Editor::UI {
    class ViewportPanel : public EditorPanel {
    public:
        void OnImGuiRender() override;

        void SetPlayModeIndicator(const bool v) { m_ShowPlayIndicator = v; }

        [[nodiscard]] float GetWidth() const { return m_ViewportWidth; }
        [[nodiscard]] float GetHeight() const { return m_ViewportHeight; }

        [[nodiscard]] int GetSelectedEntityID() const { return m_SelectedEntityID; }
        void ClearSelectedEntityID() { m_SelectedEntityID = -1; }
        [[nodiscard]] bool HadEmptyClick() const { return m_HadEmptyClick; }
        void ClearEmptyClick() { m_HadEmptyClick = false; }
        void SetUIHandleHover(const bool v) { m_UIHandleHover = v; }

        [[nodiscard]] glm::vec2 MousePosToWorld(const Rendering::Camera &camera) const;

        // position relative to the viewport top left
        [[nodiscard]] ImVec2 GetLocalMousePos() const;
        [[nodiscard]] ImVec2 GetBoundsMin() const { return m_ViewportBoundsMin; }

    private:
        float m_ViewportWidth = 1280.0f;
        float m_ViewportHeight = 720.0f;
        int m_SelectedEntityID = -1;
        bool m_ShowPlayIndicator = false;
        bool m_ExpectingPick = false;
        bool m_HadEmptyClick = false;
        bool m_UIHandleHover = false;

        ImVec2 m_ViewportBoundsMin{0, 0};
    };
} // namespace Editor::UI
