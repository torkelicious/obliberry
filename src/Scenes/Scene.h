#ifndef OBLIBERRY_SCENE_H
#define OBLIBERRY_SCENE_H
#include "ECS/ECS.h"
#include "Map/Hex.h"
#include "Renderer/Renderer.h"

/*
 * refs/inspo:
 https://docs.monogame.net/articles/tutorials/building_2d_games/17_scenes/index.html
 https://rivermanmedia.com/object-oriented-game-programming-the-scene-system/
 */

// forward decs
class InputManager;
class ResourceManager;
class Camera;

class Scene {
public:
    Scene(InputManager *input, ResourceManager *resources, Camera *camera);

    ~Scene() = default;

    void OnEnter() {
    }

    void OnExit() {
    }

    void Update(float dt);

    void Render(Renderer &renderer);

    Registry &GetRegistry() { return m_Registry; }
    HexGrid &GetGrid() { return m_Grid; }

private:
    Registry m_Registry;
    HexGrid m_Grid;

    InputManager *m_InputManager = nullptr;
    ResourceManager *m_ResourceManager = nullptr;
    Camera *m_Camera = nullptr;
};


#endif //OBLIBERRY_SCENE_H
