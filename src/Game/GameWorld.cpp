#include "GameWorld.h"

#include <cmath>

#include "Graphics/MeshFactory.h"
#include "Graphics/Renderer.h"

namespace {
glm::vec2 GridToWorldPosition(const Position &grid) {
    constexpr float size = HexMap::HEX_SPACING;
    return {
        size * std::sqrt(3.0f) * (grid.x + grid.y * 0.5f),
        size * 1.5f * grid.y
    };
}
} // namespace

GameWorld::GameWorld()
    : m_HexMesh(MeshFactory::CreatePointTopHex(HexMap::HEX_SPACING)),
      m_PlayerMesh(MeshFactory::CreateQuad()),
      m_Shader("assets/shaders/basic.vert", "assets/shaders/basic.frag"),
      m_PlayerTexture(1, "assets/textures/uv_grid.jpg", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST) {
    m_Map.Generate(10);

    m_Material.shader = &m_Shader;
    m_Material.color = {1, 1, 1, 1};

    m_PlayerMaterial.shader = &m_Shader;
    m_PlayerMaterial.texture = &m_PlayerTexture;
    m_PlayerMaterial.color = {1, 1, 1, 1};

    m_Camera.target = {0.0f, 0.0f, 0.0f};
    m_Camera.offset = {0.0f, 50.0f, 50.0f};

    m_Player.isPlayer = true;
    m_Player.GridPosition = {0.0f, 0.0f};
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

    // Render player Actor
    Transform playerTransform;
    playerTransform.Position = GridToWorldPosition(m_Player.GridPosition);
    playerTransform.Scale = {35.0f, 70.0f};
    renderer.Submit(m_PlayerMesh, m_PlayerMaterial, playerTransform);

    renderer.Flush();
}
