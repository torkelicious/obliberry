#pragma once

#include "Core/EngineContext.h"
#include "ECS/ECS.h"
#include "Scenes/Scene.h"

namespace Editor::UI {
    class EditorPanel {
    public:
        virtual ~EditorPanel() = default;

        virtual void OnImGuiRender() = 0;

        virtual void SetContext(Scenes::Scene *context, Core::EngineContext &engineCtx) {
            m_SceneContext = context;
            m_EngineContext = &engineCtx;
        }

        [[nodiscard]] bool IsHovered() const { return m_IsHovered; }

    protected:
        Scenes::Scene *m_SceneContext = nullptr;
        Core::EngineContext *m_EngineContext = nullptr;
        bool m_IsHovered = false;
    };
} // namespace Editor::UI
