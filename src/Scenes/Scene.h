#ifndef OBLIBERRY_SCENE_H
#define OBLIBERRY_SCENE_H

#include "ECS/ECS.h"
#include "Map/Hex.h"
#include "Renderer/Renderer.h"
#include "Core/Window.h"


/*
 * refs/inspo:
 https://docs.monogame.net/articles/tutorials/building_2d_games/17_scenes/index.html
 https://rivermanmedia.com/object-oriented-game-programming-the-scene-system/
 */


class InputManager;
class ResourceManager;
class Camera;

class Scene {
public:
    Scene(Window *window, InputManager *input, ResourceManager *resources, Camera *camera);

    ~Scene() = default;

    void OnEnter();

    void OnExit() {
    }

    void Update(float dt);

    void Render(Renderer &renderer);

    void GenerateTiles(int size, int percent = 50);

    void MovePlayerToCenter();

    Registry &GetRegistry() { return m_Registry; }
    HexGrid &GetGrid() { return m_Grid; }

    // for UI telemetry
    float m_PlayerSpeed = 0.15f;
    HexCoords m_SelectedHex;
    HexCoords m_PathTo;
    bool m_HasSelection = false;
    bool m_HasPathTo = false;

private:
    Registry m_Registry;
    HexGrid m_Grid;

    Window *m_Window = nullptr;
    InputManager *m_InputManager = nullptr;
    ResourceManager *m_ResourceManager = nullptr;
    Camera *m_Camera = nullptr;
};

#endif //OBLIBERRY_SCENE_H
