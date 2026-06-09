#ifndef OBLIBERRY_SCENE_H
#define OBLIBERRY_SCENE_H

#include "Core/EngineContext.h"
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
    Scene(const EngineContext &context);

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

    EngineContext m_Context;

    // still tmp
    std::shared_ptr<Mesh> m_HexMesh = nullptr;
    std::shared_ptr<Shader> m_Shader = nullptr;
    std::array<std::shared_ptr<Texture>, 6> m_PlayerTextures{};
    std::shared_ptr<Texture> m_GrassTex = nullptr;
    std::shared_ptr<Texture> m_SandTex = nullptr;
    Material m_GrassMat;
    Material m_SandMat;
    Material m_OutlineMat;
    Material m_PathToMat;
};

#endif //OBLIBERRY_SCENE_H
