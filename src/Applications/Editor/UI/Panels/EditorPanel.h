#pragma once

#include "Core/EngineContext.h"
#include "Applications/Editor/Commands/UndoManager.h"
#include "Scenes/Scene.h"

namespace Editor::Commands {
    class UndoManager;
}

namespace Editor::UI {
    class EditorPanel {
    public:
        virtual ~EditorPanel() = default;

        virtual void OnImGuiRender() = 0;

        virtual void SetContext(Scenes::Scene *context, Core::EngineContext &engineCtx, Commands::UndoManager *undoManager = nullptr) {
            m_SceneContext = context;
            m_EngineContext = &engineCtx;
            m_UndoManager = undoManager;
        }

        [[nodiscard]] bool IsHovered() const { return m_IsHovered; }

    protected:
        Scenes::Scene *m_SceneContext = nullptr;
        Core::EngineContext *m_EngineContext = nullptr;
        Commands::UndoManager *m_UndoManager = nullptr;
        bool m_IsHovered = false;
    };
} // namespace Editor::UI
