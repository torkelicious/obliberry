#pragma once
#include "Applications/Editor/UI/Panels/EditorPanel.h"

namespace Editor::UI {
    class UIPanel : public EditorPanel {
    public:
        void OnImGuiRender() override;

        void Reset() { m_SelectedElement = nullptr; }

        ::UI::UIElement *GetSelectedElement() const { return m_SelectedElement; };

    private:
        void DrawElementNode(::UI::UIElement *element);

        ::UI::UIElement *m_SelectedElement = nullptr;
        int m_AddType = 0;

        // Undo tracking for drag-to-edit
        glm::vec2 m_DragStartPos{0.0f};
        glm::vec2 m_DragStartScale{0.0f};
        std::string m_EditibNameStart;
        bool m_DraggingPos = false;
        bool m_DraggingScale = false;
        bool m_EditingName = false;
    };
} // namespace Editor::UI
