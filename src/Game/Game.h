#ifndef OBLIBERRY_GAME_H
#define OBLIBERRY_GAME_H

#include "Core/ResourceManager.h"
#include "Core/Window.h"
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

    [[nodiscard]] Camera &GetCamera() const { return *m_Camera; }
    void SetWindow(Window *win) { m_Window = win; }
    void SetInputManager(InputManager *mgr) { m_InputManager = mgr; }
    void SetCamera(Camera *camera) { m_Camera = camera; }
    void SetResourceManager(ResourceManager *rm) { m_ResourceManager = rm; }

    [[nodiscard]] bool TestFileWrite(const HexGrid &grid) const;

    [[nodiscard]] bool TestFileLoad(HexGrid &grid) const;

private:
    void Shutdown() const;

    void DrawInterface();

    GameState m_CurrentState = GameState::Gameplay;
    SceneManager m_SceneManager;

    Window *m_Window = nullptr;
    InputManager *m_InputManager = nullptr;
    Camera *m_Camera = nullptr;
    ResourceManager *m_ResourceManager = nullptr;
};

#endif //OBLIBERRY_GAME_H
