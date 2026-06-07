#ifndef OBLIBERRY_GAME_H
#define OBLIBERRY_GAME_H
#include "Core/Window.h"
#include "Map/Hex.h"
#include "Renderer/Renderer.h"


class Game {
public:
    Game() {
    };

    ~Game() { Shutdown(); }

    void Start();

    void Update(float dt);

    void Render(Renderer &renderer);

    Camera &GetCamera() { return *m_Camera; }


    void SetWindow(Window *win) { m_Window = win; }
    void SetInputManager(InputManager *mgr) { m_InputManager = mgr; }
    void SetCamera(Camera *camera) { m_Camera = camera; }

    // actual game stuff.. i.e not generic class stuff.. will prolly break :)
    HexGrid g_Grid;

    void GenerateTiles(HexGrid &map, int size, int percent = 50);

    void MovePlayerToCenter();

    bool TestFileWrite(const HexGrid &grid) const;

    bool TestFileLoad(HexGrid &grid) const;

private:
    void Shutdown() const;

    Window *m_Window = nullptr;
    InputManager *m_InputManager = nullptr;
    Camera *m_Camera = nullptr;
};


#endif //OBLIBERRY_GAME_H
