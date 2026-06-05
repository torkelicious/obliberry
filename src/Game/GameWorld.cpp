#include "GameWorld.h"

#include <iostream>

#include "Core/ResourceManager.h"
#include "ECS/Components.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/PlayerInputComponent.h"
#include "ECS/Components/SpriteComponent.h"
#include "ECS/Components/VelocityComponent.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/Renderer.h"
#include "Graphics/Texture.h"
#include "Graphics/Material.h"
#include "Graphics/Shader.h"
#include "Graphics/Mesh.h"

// Rendering order is handled by the Renderer via hybrid Z-layer + Y-sorting:
//   Z = 0.0 -> ground (hex tiles), Z = 0.1 -> entities (player), Z = 1.0 -> UI.
//   Within a Z-layer, larger Y is drawn first (back-to-front for isometric view). ?? something i guess


GameWorld::GameWorld(InputManager &input) {
    m_Input = &input;
    m_HexMesh = ResourceManager::Load<Mesh>("hex_mesh");

    m_Shader = ResourceManager::Load<Shader>(
        "basic_shader",
        "assets/shaders/basic.vert",
        "assets/shaders/basic.frag"
    );

    m_HexTexture = ResourceManager::Load<Texture>(
        "gradient_tex",
        "assets/textures/HexGrad.png",
        GL_NEAREST_MIPMAP_NEAREST,
        GL_NEAREST
    );

    m_HexMesh->Upload(
        MeshFactory::CreatePointTopHex(HexMap::HEX_SPACING)
    );

    auto *playerTex = ResourceManager::Load<Texture>(
        "player_tex",
        "assets/textures/uv_grid.jpg",
        GL_NEAREST_MIPMAP_NEAREST,
        GL_NEAREST
    );

    auto *playerMesh = ResourceManager::Load<Mesh>("player_mesh");

    playerMesh->Upload(MeshFactory::CreateQuad());

    auto *playerMat = new Material(
        m_Shader,
        playerTex,
        {1, 1, 1, 1}
    );

    m_Map.Generate(10);

    m_Player.AddComponent<Transform>();
    auto *transform = m_Player.GetComponent<Transform>();
    transform->Scale = {35.0f, 70.0f, 1.0f};
    transform->Position = {0.0f, 0.0f, 35.0f + 0.1f};
    transform->Rotation = {glm::radians(90.0f), 0.0f, 0.0f};

    m_Player.AddComponent<MeshComponent>();
    m_Player.GetComponent<MeshComponent>()->mesh = playerMesh;

    m_Player.AddComponent<MaterialComponent>();
    m_Player.GetComponent<MaterialComponent>()->material = playerMat;

    m_Player.AddComponent<PlayerInput>();
    m_Player.AddComponent<Velocity>();

    m_Player.AddComponent<Sprite>();
    m_Player.GetComponent<Sprite>()->SpriteTexture = playerTex;

    m_Material.shader = m_Shader;
    m_Material.texture = m_HexTexture;

    m_Camera.target = {0.0f, 0.0f, 0.0f};
    m_Camera.offset = {0.0f, -100.0f, 100.0f};
    m_Camera.isometric = true;
}


void GameWorld::Update(float dt) {
    auto *transform = m_Player.GetComponent<Transform>();
    auto *velocity = m_Player.GetComponent<Velocity>();

    if (!transform || !velocity)
        return;

    constexpr float playerSpeed = 160.0f;

    transform->Position += glm::vec3(
        velocity->Value * playerSpeed * dt,
        0.0f
    );

    m_Camera.target = {
        transform->Position.x,
        transform->Position.y,
        0.0f
    };

    //   glm::vec2 scroll = m_Player.GetComponent<PlayerInput>().

    m_Camera.AddZoom(m_Input->scrollY * 0.1f);
    //std::cout << "scrolly:" << m_Input->scrollY << std::endl;
    m_Input->scrollY = 0;

    bool isMoving =
            std::abs(velocity->Value.x) > 0.001f ||
            std::abs(velocity->Value.y) > 0.001f;

    if (!isMoving) {
        transform->Position = GetClosestHexPosition(*transform);
    }
    transform->Position.z = 0.1f;
    // idk wat im doing
}


void GameWorld::Render(Renderer &renderer) {
    renderer.BeginFrame(m_Camera);

    for (const auto &tile: m_Map.tiles) {
        Transform t;
        t.Position = {tile.WorldPos.x, tile.WorldPos.y, 0.0f};
        t.Scale = {1.0f, 1.0f, 1.0f};
        t.Rotation = {0.0f, 0.0f, 0.0f};

        renderer.Submit(*m_HexMesh, m_Material, t);
    }
    renderer.Submit(
        *m_Player.GetComponent<MeshComponent>()->mesh,
        *m_Player.GetComponent<MaterialComponent>()->material,
        *m_Player.GetComponent<Transform>());

    renderer.Flush();
}

void GameWorld::Shutdown() {
    ResourceManager::Shutdown();
}

glm::vec3 GameWorld::GetClosestHexPosition(const Transform &t) {
    if (m_Map.tiles.empty())
        return t.Position;

    const glm::vec3 *closest = &m_Map.tiles[0].WorldPos;
    float bestDist = glm::length(*closest - t.Position);

    for (const auto &tile: m_Map.tiles) {
        float dist = glm::length(tile.WorldPos - t.Position);

        if (dist < bestDist) {
            bestDist = dist;
            closest = &tile.WorldPos;
        }
    }

    return *closest;
}
