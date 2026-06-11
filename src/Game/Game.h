#ifndef OBLIBERRY_GAME_H
#define OBLIBERRY_GAME_H

#include <optional>
#include <string>
#include "Core/ResourceManager.h"
#include "Core/Window.h"
#include "Core/EngineContext.h"
#include "Renderer/Renderer.h"
#include "../Scenes/SceneManager.h"
#include "Map/Hex.h"

enum class GameState { MainMenu, Gameplay, Paused, EditorMode };


class Game {
public:
    Game() = default;

    ~Game() { Shutdown(); }

    void Start();

    void Update(float dt);

    void Render();

    [[nodiscard]] Camera &GetCamera() const { return *m_Context.camera; }
    void SetContext(const EngineContext &context) { m_Context = context; }

    [[nodiscard]] bool TestFileWrite(const HexGrid &grid, const std::string &path) const;

    [[nodiscard]] bool TestFileLoad(HexGrid &grid, const std::string &path) const;

private:
    void Shutdown() const;

    void DrawInterface();

    GameState m_CurrentState = GameState::Gameplay;
    SceneManager m_SceneManager;

    EngineContext m_Context;
    std::optional<std::string> m_PendingSceneLoad;
};

#endif //OBLIBERRY_GAME_H
