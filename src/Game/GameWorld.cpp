#include "GameWorld.h"

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

// TODO:
//  Fix rendering order (currently relying on depth buffer which breaks in ortho)
//  Decide final layering system .-. either Y-sorting (recommended, and most probable) or Z-layers (0 = tiles, 0.1 = player, 1 = UI) ???????
//  Ensure all objects use a single consistent world space (i be mixing hex/grid transforms in render because i dum dum)
//  Verify shader stuffs
//  reosurcemanager seems 2 be done ;:))))
// make gaem


GameWorld::GameWorld() {
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
    transform->Scale = {35, 70};
    transform->Position = {0, 0, 0.1f};

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
    m_Camera.offset = {0.0f, 50.0f, 50.0f};
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
}


void GameWorld::Render(Renderer &renderer) {
    renderer.BeginFrame(m_Camera);

    for (const auto &tile: m_Map.tiles) {
        Transform t;
        t.Position = {tile.WorldPos.x, tile.WorldPos.y, 0.0f};
        t.Scale = {1.0f, 1.0f};
        t.rotation = 0.0f;

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
