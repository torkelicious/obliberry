#pragma once

#include <optional>
#include "Core/EngineContext.h"
#include "../Scenes/SceneManager.h"

enum class GameState : uint8_t { MainMenu, Gameplay, Paused, EditorMode };


class Game {
public:
    Game() = default;

    ~Game() { Shutdown(); }

    void Start();

    void Update(float dt);

    void Render() const;

    [[nodiscard]] Camera &GetCamera() const { return *m_Context.camera; }
    void SetContext(const EngineContext &context) { m_Context = context; }

private:
    static void Shutdown();

    void DrawInterface();

    GameState m_CurrentState = GameState::Gameplay;
    SceneManager m_SceneManager;

    EngineContext m_Context;
    std::optional<SceneProperties> m_PendingSceneLoad;
};

