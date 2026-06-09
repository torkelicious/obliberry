#ifndef OBLIBERRY_GAME_H
#define OBLIBERRY_GAME_H

#include "Core/ResourceManager.h"
#include "Core/Window.h"
#include "Core/EngineContext.h"
#include "Renderer/Renderer.h"
#include "Scenes/SceneManager.h"
#include "Map/Hex.h"

enum class GameState { MainMenu, Gameplay, Paused, EditorMode }; // for later use im still planning things out

class Game {
public:
    Game() = default;

    ~Game() { Shutdown(); }

    void Start();

    void Update(float dt);

    void Render(Renderer &renderer);

    [[nodiscard]] Camera &GetCamera() const { return *m_Context.camera; }
    void SetContext(const EngineContext &context) { m_Context = context; }

    [[nodiscard]] bool TestFileWrite(const HexGrid &grid) const;

    [[nodiscard]] bool TestFileLoad(HexGrid &grid) const;

private:
    void Shutdown() const;

    void DrawInterface();

    GameState m_CurrentState = GameState::Gameplay;
    SceneManager m_SceneManager;

    EngineContext m_Context;
};

#endif //OBLIBERRY_GAME_H
