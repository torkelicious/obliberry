#pragma once

#include <optional>
#include "Core/EngineContext.h"
#include "Scenes/SceneManager.h"
#include "Core/ApplicationLayer.h"

namespace Game {
    class GameLayer : public Core::ApplicationLayer {
    public:
        void Init(Core::EngineContext &ctx) override;

        void Update(float dt) override;

        void Render() override;

        void Shutdown() override;

        [[nodiscard]] Rendering::Camera &GetCamera() const { return *m_Context->camera; }
        void SetContext(Core::EngineContext &context) { m_Context = &context; }

    private:
        void DrawInterface() const;

        bool m_GameIsRunning = true;
        Scenes::SceneManager m_SceneManager;

        Core::EngineContext *m_Context = nullptr;
        std::optional<Scenes::SceneProperties> m_PendingSceneLoad;
    };
} // namespace Game
