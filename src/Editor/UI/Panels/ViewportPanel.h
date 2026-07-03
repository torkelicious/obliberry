#pragma once

#include "EditorPanel.h"

namespace Editor::UI {
    class ViewportPanel : public EditorPanel {
    public:
        void OnImGuiRender() override;

        void SetPlayModeIndicator(const bool v) { m_ShowPlayIndicator = v; }

        [[nodiscard]] float GetWidth() const { return m_ViewportWidth; }
        [[nodiscard]] float GetHeight() const { return m_ViewportHeight; }

        [[nodiscard]] int GetSelectedEntityID() const { return m_SelectedEntityID; }
        void ClearSelectedEntityID() { m_SelectedEntityID = -1; }

    private:
        float m_ViewportWidth = 1280.0f;
        float m_ViewportHeight = 720.0f;
        int m_SelectedEntityID = -1;
        bool m_ShowPlayIndicator = false;
    };
} // namespace Editor::UI
