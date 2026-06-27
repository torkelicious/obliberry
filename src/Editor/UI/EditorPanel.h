#pragma once

#include "Core/EngineContext.h"
#include "ECS/ECS.h"
#include "Scenes/Scene.h"

class EditorPanel {
public:
    virtual ~EditorPanel() = default;

    virtual void OnImGuiRender() = 0;

    virtual void SetContext(Scene *context, EngineContext &engineCtx) {
        m_SceneContext = context;
        m_EngineContext = &engineCtx;
    }

    [[nodiscard]] bool IsHovered() const { return m_IsHovered; }

protected:
    Scene *m_SceneContext = nullptr;
    EngineContext *m_EngineContext = nullptr;
    bool m_IsHovered = false;
};
