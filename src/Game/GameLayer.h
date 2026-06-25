#pragma once

#include <optional>
#include "Core/EngineContext.h"
#include "../Scenes/SceneManager.h"
#include "../Core/ApplicationLayer.h"

class GameLayer : public ApplicationLayer {
public:
    void Init(EngineContext &ctx) override;

    void Update(float dt) override;

    void Render() override;

    void Shutdown() override;

    [[nodiscard]] Camera &GetCamera() const { return *m_Context.camera; }
    void SetContext(const EngineContext &context) { m_Context = context; }

private:
    void DrawInterface();

    bool m_GameIsRunning = true;
    SceneManager m_SceneManager;

    EngineContext m_Context;
    std::optional<SceneProperties> m_PendingSceneLoad;
};
