#ifndef ISOMETRICGAME_GAMEWORLD_H
#define ISOMETRICGAME_GAMEWORLD_H

#include "Graphics/Camera.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture.h"
#include "Actor.h"
#include "Map/HexMap.h"

class Renderer;

class GameWorld {
public:
    GameWorld();

    void Render(Renderer &renderer);

    Camera &GetCamera() { return m_Camera; }
    const Camera &GetCamera() const { return m_Camera; }

    Actor &GetPlayer() { return m_Player; }
    const Actor &GetPlayer() const { return m_Player; }

private:
    Camera m_Camera;
    Mesh m_HexMesh;
    Mesh m_PlayerMesh;
    Shader m_Shader;
    Material m_Material;
    Material m_PlayerMaterial;
    Texture m_PlayerTexture;
    HexMap m_Map;
    Actor m_Player;
};

#endif //ISOMETRICGAME_GAMEWORLD_H
