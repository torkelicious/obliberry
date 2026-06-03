#ifndef ISOMETRICGAME_GAMEWORLD_H
#define ISOMETRICGAME_GAMEWORLD_H

#include "Graphics/Camera.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"
#include "Graphics/Shader.h"
#include "Map/HexMap.h"

class Renderer;

class GameWorld {
public:
    GameWorld();

    void Render(Renderer &renderer);

    Camera &GetCamera() { return m_Camera; }
    const Camera &GetCamera() const { return m_Camera; }

private:
    Camera m_Camera;
    Mesh m_HexMesh;
    Shader m_Shader;
    Material m_Material;
    HexMap m_Map;
};

#endif //ISOMETRICGAME_GAMEWORLD_H
