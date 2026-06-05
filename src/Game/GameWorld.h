#ifndef ISOMETRICGAME_GAMEWORLD_H
#define ISOMETRICGAME_GAMEWORLD_H

#include "Graphics/Camera.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture.h"
#include "ECS/ECS.h"
#include "Map/HexMap.h"

class Renderer;

class GameWorld {
public:
    GameWorld();

    ~GameWorld() {
        Shutdown();
    }

    void Update(float dt);

    void Render(Renderer &renderer);

    void Shutdown();

    Camera &GetCamera() { return m_Camera; }

    Entity &GetPlayer() { return m_Player; }

    //const Actor &GetPlayer() const { return m_Player; }

private:
    Camera m_Camera;
    Mesh *m_HexMesh = nullptr;
    //    Mesh m_PlayerMesh;
    Shader *m_Shader = nullptr;
    Material m_Material;
    //   Material m_PlayerMaterial;
    Texture *m_HexTexture = nullptr;
    //  Texture m_PlayerTexture;
    HexMap m_Map;
    Entity m_Player;
};

#endif //ISOMETRICGAME_GAMEWORLD_H
