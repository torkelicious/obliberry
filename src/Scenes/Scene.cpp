#include "Scene.h"
#include "Core/ResourceManager.h"
#include "Core/Constants.h"
#include "Core/Utils.h"
#include "Renderer/MeshFactory.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/MovementSystem.h"
#include "ECS/Systems/PlayerInputSystem.h"
#include "ECS/Systems/RenderSystem.h"
#include <array>
#include <cstdlib>
#include "ECS/Systems/InteractionSystem.h"
#include "ECS/Systems/PlayerControlSystem.h"
#include "ECS/Systems/AISystem.h"
#include "ECS/Systems/MapRenderSystem.h"
#include "ECS/Systems/SpriteBillboardSystem.h"

/*
!!! GET LOGIC OUT OF SCENE BEFORE IT BECOMES A MESS !!!
TODO:
 * decouple systems & cleanup dependencies
 *      create an EngineContext struct (window, input, resources, camera, dt) to stop passing raw pointers everywhere
 *      get camera panning & zoom math out of Scene::Update - kinda its a system now
 *      implement a ComponentRegistry to map component names to ImGui editors & serializers blah blah
 *      turn entities into data-driven "Prototypes" (stats/visual data loaded from disk instead of hardcoded)
 *
 IMPLEMENT:
 *      proper standalone systems - pretty muich done
 *
 BEHAVIOURS?
 *      create some sort of Logic/Script Component that just holds a string IDs?? still planning iomplemetnatoin of this..
 *      bind gameplay functions to those string IDs in a central behavior manager map
 *      make engine systems trigger hardcoded procedures
 *      allows changing logic via editor dropdowns without dealing with custom compilers or whatever
 ASSETS & STUFF:
 *      nuke the temp anon namespace globals in Scene.cpp
 *      load materials dynamically via string IDs / handles from ResourceManager
 *      flesh out SceneSerializer to serialize/deserialize the HexGrid, ECS entities, and their assigned Script ID strings?
 *
 *
 ENTITY LIFE:
 *      figure out cross-scene handling for player data (stats/inventory?)
 *      maybe a DontDestroyOnLoad component tag to migrate entities between scene registries
 *
 */


Scene::Scene(const EngineContext &context)
    : m_Context(context) {
}

/*
 * none of this stuff should be here entligen but it is housed here for now while
 * i start work on fleshing out the rest of the stuff and start work on all i must do...
 * this class is supposed to be a more abstract data driven type thing defined by the serialization
 * of oblvl files but this is not implemented yet so i am housing everything here TEMPORARIALY!!!!
 * a ton of work needs to be done ..
*/

// ALOT OF THIS IS STILL TEMPORARY!!! :P
void Scene::OnEnter() {
    m_PlayerTextures[0] = m_Context.resources->Load<Texture>(
        "p_east", PathUtils::Join(TEXTURE_PATH, "player/player_e.png"));
    m_PlayerTextures[1] = m_Context.resources->Load<Texture>("p_north_east",
                                                             PathUtils::Join(TEXTURE_PATH, "player/player_ne.png"));
    m_PlayerTextures[2] = m_Context.resources->Load<Texture>("p_north_west",
                                                             PathUtils::Join(TEXTURE_PATH, "player/player_nw.png"));
    m_PlayerTextures[3] = m_Context.resources->Load<Texture>(
        "p_west", PathUtils::Join(TEXTURE_PATH, "player/player_w.png"));
    m_PlayerTextures[4] = m_Context.resources->Load<Texture>("p_south_west",
                                                             PathUtils::Join(TEXTURE_PATH, "player/player_sw.png"));
    m_PlayerTextures[5] = m_Context.resources->Load<Texture>("p_south_east",
                                                             PathUtils::Join(TEXTURE_PATH, "player/player_se.png"));

    m_Shader = m_Context.resources->Load<Shader>("base_shader", PathUtils::Join(SHADER_PATH, "base.vert"),
                                                 PathUtils::Join(SHADER_PATH, "base.frag"));
    m_GrassTex = m_Context.resources->Load<Texture>("grass_tex", PathUtils::Join(TEXTURE_PATH, "HexGrass.png"));
    m_SandTex = m_Context.resources->Load<Texture>("sand_tex", PathUtils::Join(TEXTURE_PATH, "HexSand.png"));

    std::shared_ptr<Mesh> playerMesh = m_Context.resources->LoadFromFactory<Mesh>("p_mesh", [] {
        auto data = MeshFactory::CreateQuad();
        return std::make_shared<Mesh>(data.vertices, data.indices);
    });

    m_HexMesh = m_Context.resources->LoadFromFactory<Mesh>("hex_mesh", [] {
        auto data = MeshFactory::CreatePointTopHex(0.5f);
        return std::make_shared<Mesh>(data.vertices, data.indices);
    });

    m_GrassMat = {m_Shader, m_GrassTex, {1.0f, 1.0f, 1.0f, 1.f}};
    m_SandMat = {m_Shader, m_SandTex, {1.0f, 1.0f, 1.0f, 1.0f}};
    m_OutlineMat = {m_Shader, nullptr, {1.0f, 0.0f, 0.0f, 0.5f}};
    m_PathToMat = {m_Shader, nullptr, {1.0f, 1.0f, 1.0f, 0.5f}};

    GenerateTiles(20, 50);

    // Spawn Player
    Entity player = Entity(m_Registry.CreateEntity(), &m_Registry);
    player.AddComponent<MeshComponent>(MeshComponent{playerMesh});
    player.AddComponent<MaterialComponent>(MaterialComponent{
        std::make_shared<Material>(Material{m_Shader, m_PlayerTextures[0], {1.0f, 1.0f, 1.0f, 1.0f}})
    });
    player.AddComponent<TransformComponent>(TransformComponent{});
    player.AddComponent<MovementComponent>(MovementComponent{});
    player.AddComponent<PlayerInputComponent>(PlayerInputComponent{});

    if (auto *pTrans = player.GetComponent<TransformComponent>()) {
        pTrans->transform.SetPosition({0.0f, 0.0f, 0.05f});
        pTrans->transform.SetScale({0.5f, 1.0f, 1.0f});
    }

    // Spawn NPC
    Entity npc = Entity(m_Registry.CreateEntity(), &m_Registry);
    npc.AddComponent<MeshComponent>(MeshComponent{playerMesh});
    npc.AddComponent<MaterialComponent>(MaterialComponent{
        std::make_shared<Material>(Material{m_Shader, m_PlayerTextures[0], {1.0f, 0.5f, 0.5f, 1.0f}})
    });
    npc.AddComponent<TransformComponent>(TransformComponent{});
    npc.AddComponent<MovementComponent>(MovementComponent{});

    if (auto *npcTrans = npc.GetComponent<TransformComponent>()) {
        npcTrans->transform.SetPosition({2.0f, 2.0f, 0.05f});
        npcTrans->transform.SetScale({1.0f, 1.0f, 1.0f});
    }
}

void Scene::Update(float dt) {
    m_Context.deltaTime = dt;
    glm::vec2 worldPos = InteractionSystem::Update(
        m_Registry,
        m_Context,
        m_Grid,
        m_SelectedHex,
        m_HasSelection
    );

    PlayerControlSystem::Update(m_Registry, m_Context, m_Grid, worldPos, m_PlayerSpeed, m_SelectedHex, m_HasSelection,
                                m_PathTo, m_HasPathTo, m_PlayerTextures);
    AISystem::Update(m_Registry, m_Grid, m_PlayerSpeed, m_PlayerTextures);
    InputSystem::Update(m_Registry, *m_Context.input);
    MovementSystem::Update(m_Registry, dt, m_Grid);
}


void Scene::Render(Renderer &renderer) {
    MapRenderSystem::Render(
        renderer,
        m_Grid,
        m_GrassMat,
        m_SandMat,
        m_OutlineMat,
        m_PathToMat,
        m_HexMesh,
        m_SelectedHex,
        m_HasSelection,
        m_PathTo,
        m_HasPathTo
    );

    if (m_Context.camera != nullptr) {
        m_Registry.ForEach<TransformComponent>([&](Entity entity, TransformComponent *transComp) {
            glm::vec3 pos = transComp->transform.GetPosition();
            glm::vec3 scale = transComp->transform.GetScale();
            transComp->transform.SetCustomMatrix(
                SpriteBillboardSystem::GetBillboardMatrix(pos, scale.x, scale.y, *m_Context.camera));
        });
    }

    // all normal entities
    RenderSystem::Render(m_Registry, renderer);
}

void Scene::GenerateTiles(int size, int percent) {
    if (!m_Grid.tiles.empty()) m_Grid.tiles.clear();
    size = size / 2;
    for (int q = -size; q < size; q++) {
        for (int r = -size; r < size; r++) {
            m_Grid.EmplaceTile({q, r}, (std::rand() % 100 < percent) ? TileType::Sand : TileType::Grass);
        }
    }
}

void Scene::MovePlayerToCenter() {
    m_Registry.ForEach<PlayerInputComponent, TransformComponent, MovementComponent>(
        [&](Entity e, PlayerInputComponent *p, TransformComponent *trans, MovementComponent *move) {
            MovementSystem::CancelPath(move);
            glm::vec3 pos = trans->transform.GetPosition();
            pos.x = 0.0f;
            pos.y = 0.0f;
            trans->transform.SetPosition(pos);
        }
    );
}
