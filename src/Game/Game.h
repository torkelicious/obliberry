#ifndef OBLIBERRY_GAME_H
#define OBLIBERRY_GAME_H

#include <vector>
#include "Core/ResourceManager.h"
#include "Core/Window.h"
#include "ECS/ECS.h"
#include "Map/Hex.h"
#include "Renderer/Renderer.h"

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

    HexGrid g_Grid;

    // ECS
    Registry m_Registry;
    Entity m_player{};
    Entity m_NPC{};

    void SetResourceManager(ResourceManager *rm) { m_ResourceManager = rm; }

    void GenerateTiles(HexGrid &map, int size, int percent = 50);

    void MovePlayerToCenter();

    [[nodiscard]] bool TestFileWrite(const HexGrid &grid) const;

    [[nodiscard]] bool TestFileLoad(HexGrid &grid) const;

private:
    void Shutdown() const;

    Window *m_Window = nullptr;
    InputManager *m_InputManager = nullptr;
    Camera *m_Camera = nullptr;
    ResourceManager *m_ResourceManager = nullptr;
};

#endif //OBLIBERRY_GAME_H
