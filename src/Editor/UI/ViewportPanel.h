#pragma once

#include "EditorPanel.h"

class ViewportPanel : public EditorPanel {
public:
    void OnImGuiRender() override;

    float GetViewportWidth() const { return m_ViewportWidth; }
    float GetViewportHeight() const { return m_ViewportHeight; }

private:
    float m_ViewportWidth = 1280.0f;
    float m_ViewportHeight = 720.0f;
};
