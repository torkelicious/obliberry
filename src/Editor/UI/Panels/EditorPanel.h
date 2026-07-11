#pragma once

#include "Core/EngineContext.h"
#include "Scenes/Scene.h"

namespace Editor {
    class UndoManager;
}

namespace Editor::UI {
    class EditorPanel {
    public:
        virtual ~EditorPanel() = default;

        virtual void OnImGuiRender() = 0;

        virtual void SetContext(Scenes::Scene *context, Core::EngineContext &engineCtx, Editor::UndoManager *undoManager = nullptr) {
            m_SceneContext = context;
            m_EngineContext = &engineCtx;
            m_UndoManager = undoManager;
        }

        [[nodiscard]] bool IsHovered() const { return m_IsHovered; }

    protected:
        Scenes::Scene *m_SceneContext = nullptr;
        Core::EngineContext *m_EngineContext = nullptr;
        Editor::UndoManager *m_UndoManager = nullptr;
        bool m_IsHovered = false;
    };
} // namespace Editor::UI
