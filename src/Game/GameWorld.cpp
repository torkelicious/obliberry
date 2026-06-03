#include "GameWorld.h"

#include "Graphics/MeshFactory.h"
#include "Graphics/Renderer.h"

GameWorld::GameWorld()
    : m_HexMesh(MeshFactory::CreatePointTopHex(HexMap::HEX_SPACING)),
      m_Shader("assets/shaders/basic.vert", "assets/shaders/basic.frag") {
    m_Map.Generate(10);

    m_Material.shader = &m_Shader;
    m_Material.color = {1, 1, 1, 1};

    m_Camera.target = {0.0f, 0.0f, 0.0f};
    m_Camera.offset = {0.0f, 50.0f, 50.0f};
}

void GameWorld::Render(Renderer &renderer) {
    renderer.BeginFrame(m_Camera);

    // render hex tile grid map thing
    for (const auto &tile: m_Map.tiles) {
        Transform t;
        t.Position = tile.WorldPos;
        t.Scale = {1.0f, 1.0f};
        t.rotation = 0.0f;
        renderer.Submit(m_HexMesh, m_Material, t);
    }

    renderer.Flush();
}
