#include "Scene.h"
#include "Core/ResourceManager.h"
#include "Core/Constants.h"
#include "Core/Utils.h"
#include "Renderer/MeshFactory.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/Movement.h"
#include "ECS/Systems/PlayerInput.h"
#include "ECS/Systems/RenderSystem.h"
#include <array>
#include <cstdlib>

/*
!!! GET LOGIC OUT OF SCENE BEFORE IT BECOMES A MESS !!!
TODO:
 * decouple systems & cleanup dependencies
 *      create an EngineContext struct (window, input, resources, camera, dt) to stop passing raw pointers everywhere
 *      get camera panning & zoom math out of Scene::Update
 *      extract mouse-to-world hex selection into an interaction system?
 *      implement a ComponentRegistry to map component names to ImGui editors & serializers blah blah
 *      turn entities into data-driven "Prototypes" (stats/visual data loaded from disk instead of hardcoded)
 *
 IMPLEMENT:
 *      proper standalone systems
 *      move player input registry loop into some sort of PlayerControlSystem?
 *      move NPC random wandering / pathing into AISystem or something
 *      get map rendering & billboard loops out of Scene::Render into a system
 *
 BEHAVIOURS?
 *      create some sort of Logic/Script Component that just holds a string IDs
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


// !!! ALL OF THIS STUFF IS TEMPORARY AS I FIGURE THINGS OUT !!!
namespace {
    std::shared_ptr<Mesh> g_HexMesh = nullptr;
    std::shared_ptr<Shader> g_Shader = nullptr;
    std::array<std::shared_ptr<Texture>, 6> g_PlayerTextures{};

    std::shared_ptr<Texture> g_GrassTex = nullptr;
    std::shared_ptr<Texture> g_SandTex = nullptr;

    Material g_GrassMat;
    Material g_SandMat;
    Material g_OutlineMat;
    Material g_PathToMat;
}

Scene::Scene(Window *window, InputManager *input, ResourceManager *resources, Camera *camera)
    : m_Window(window), m_InputManager(input), m_ResourceManager(resources), m_Camera(camera) {
}

/*
 * none of this stuff should be here entligen but it is housed here for now while
 * i start work on fleshing out the rest of the stuff and start work on all i must do...
 * this class is supposed to be a more abstract data driven type thing defined by the serialization
 * of oblvl files but this is not implemented yet so i am housing everything here TEMPORARIALY!!!!
 * a ton of work needs to be done ..
*/

void Scene::OnEnter() {
    // load Assets
    g_PlayerTextures[0] = m_ResourceManager->Load<Texture>(
        "p_east", PathUtils::Join(TEXTURE_PATH, "player/player_e.png"));
    g_PlayerTextures[1] = m_ResourceManager->Load<Texture>("p_north_east",
                                                           PathUtils::Join(TEXTURE_PATH, "player/player_ne.png"));
    g_PlayerTextures[2] = m_ResourceManager->Load<Texture>("p_north_west",
                                                           PathUtils::Join(TEXTURE_PATH, "player/player_nw.png"));
    g_PlayerTextures[3] = m_ResourceManager->Load<Texture>(
        "p_west", PathUtils::Join(TEXTURE_PATH, "player/player_w.png"));
    g_PlayerTextures[4] = m_ResourceManager->Load<Texture>("p_south_west",
                                                           PathUtils::Join(TEXTURE_PATH, "player/player_sw.png"));
    g_PlayerTextures[5] = m_ResourceManager->Load<Texture>("p_south_east",
                                                           PathUtils::Join(TEXTURE_PATH, "player/player_se.png"));

    g_Shader = m_ResourceManager->Load<Shader>("base_shader", PathUtils::Join(SHADER_PATH, "base.vert"),
                                               PathUtils::Join(SHADER_PATH, "base.frag"));
    g_GrassTex = m_ResourceManager->Load<Texture>("grass_tex", PathUtils::Join(TEXTURE_PATH, "HexGrass.png"));
    g_SandTex = m_ResourceManager->Load<Texture>("sand_tex", PathUtils::Join(TEXTURE_PATH, "HexSand.png"));

    std::shared_ptr<Mesh> playerMesh = m_ResourceManager->LoadFromFactory<Mesh>("p_mesh", [] {
        auto data = MeshFactory::CreateQuad();
        return std::make_shared<Mesh>(data.vertices, data.indices);
    });

    g_HexMesh = m_ResourceManager->LoadFromFactory<Mesh>("hex_mesh", [] {
        auto data = MeshFactory::CreatePointTopHex(0.5f);
        return std::make_shared<Mesh>(data.vertices, data.indices);
    });

    g_GrassMat = {g_Shader, g_GrassTex, {1.0f, 1.0f, 1.0f, 1.f}};
    g_SandMat = {g_Shader, g_SandTex, {1.0f, 1.0f, 1.0f, 1.0f}};
    g_OutlineMat = {g_Shader, nullptr, {1.0f, 0.0f, 0.0f, 0.5f}};
    g_PathToMat = {g_Shader, nullptr, {1.0f, 1.0f, 1.0f, 0.5f}};

    // generate map
    GenerateTiles(20, 50);

    // spawn entities using scene registry
    Entity player = Entity(m_Registry.CreateEntity(), &m_Registry);
    player.AddComponent<MeshComponent>(MeshComponent{playerMesh});
    player.AddComponent<MaterialComponent>(MaterialComponent{
        std::make_shared<Material>(Material{g_Shader, g_PlayerTextures[0], {1.0f, 1.0f, 1.0f, 1.0f}})
    });
    player.AddComponent<TransformComponent>(TransformComponent{});
    player.AddComponent<MovementComponent>(MovementComponent{});
    player.AddComponent<PlayerInputComponent>(PlayerInputComponent{});

    if (auto *pTrans = player.GetComponent<TransformComponent>()) {
        pTrans->transform.SetPosition({0.0f, 0.0f, 0.05f});
        pTrans->transform.SetScale({0.5f, 1.0f, 1.0f});
    }

    Entity npc = Entity(m_Registry.CreateEntity(), &m_Registry);
    npc.AddComponent<MeshComponent>(MeshComponent{playerMesh});
    npc.AddComponent<MaterialComponent>(MaterialComponent{
        std::make_shared<Material>(Material{g_Shader, g_PlayerTextures[0], {1.0f, 0.5f, 0.5f, 1.0f}})
    });
    npc.AddComponent<TransformComponent>(TransformComponent{});
    npc.AddComponent<MovementComponent>(MovementComponent{});

    if (auto *npcTrans = npc.GetComponent<TransformComponent>()) {
        npcTrans->transform.SetPosition({2.0f, 2.0f, 0.05f});
        npcTrans->transform.SetScale({1.0f, 1.0f, 1.0f});
    }
}

void Scene::Update(float dt) {
    float windowWidth = static_cast<float>(m_Window->GetWidth());
    float windowHeight = static_cast<float>(m_Window->GetHeight());

    // Camera Panning
    if (m_InputManager->scrollY != 0.0) {
        m_Camera->Zoom += static_cast<float>(m_InputManager->scrollY) * ZOOM_SPEED;
        m_Camera->Zoom = std::clamp(m_Camera->Zoom, 0.5f, 5.0f);
    }

    float edgeMargin = 20.0f;
    glm::vec2 screenPan(0.0f);
    HexMath::Point mousePos = {
        static_cast<float>(m_InputManager->mousePosX), static_cast<float>(m_InputManager->mousePosY)
    };

    if (mousePos.x <= edgeMargin) screenPan.x -= 1.0f;
    else if (mousePos.x >= windowWidth - edgeMargin) screenPan.x += 1.0f;
    if (mousePos.y <= edgeMargin) screenPan.y += 1.0f;
    else if (mousePos.y >= windowHeight - edgeMargin) screenPan.y -= 1.0f;

    if (glm::length(screenPan) > 0.0f) {
        screenPan = glm::normalize(screenPan);
        glm::mat4 invRot = glm::inverse(m_Camera->GetRotation());
        glm::vec4 worldPan = invRot * glm::vec4(screenPan.x, screenPan.y, 0.0f, 0.0f);
        m_Camera->Position += glm::vec2(worldPan.x, worldPan.y) * PAN_SPEED * dt * (1.0f / m_Camera->Zoom);
    }

    glm::vec2 worldPos = m_Camera->MouseToWorld(mousePos.x, mousePos.y, windowWidth, windowHeight);

    // Global Hex Selection
    HexCoords hexPosOnMpos = HexMath::PixelToHex({worldPos.x, worldPos.y});
    if (m_Grid.Get(hexPosOnMpos) == nullptr) {
        m_HasSelection = false;
    } else {
        m_SelectedHex = hexPosOnMpos;
        m_HasSelection = true;
    }

    // Player Input
    m_Registry.ForEach<PlayerInputComponent, TransformComponent, MovementComponent, MaterialComponent>(
        [&](Entity entity, PlayerInputComponent *input, TransformComponent *trans, MovementComponent *move,
            MaterialComponent *mat) {
            move->timePerStep = m_PlayerSpeed;
            glm::vec2 pPos = glm::vec2(trans->transform.GetPosition());

            glm::vec2 targetDir;
            bool hasTarget = false;

            if (move->isMoving && move->currentPathIndex < move->currentPath.size()) {
                HexCoords nextHex = move->currentPath[move->currentPathIndex];
                targetDir = HexMath::HexToWorld(nextHex) - pPos;
                hasTarget = true;
            } else {
                targetDir = worldPos - pPos;
                if (glm::length(targetDir) > (HEX_SIZE - 0.005f)) hasTarget = true;
            }

            if (hasTarget) {
                float degrees = glm::degrees(glm::atan(targetDir.y, targetDir.x));
                if (degrees < 0.0f) degrees += 360.0f;
                mat->material->texture = g_PlayerTextures[static_cast<int>(std::lround(degrees / 60.0f)) % 6];
            }

            if (m_InputManager->IsMousePressed(GLFW_MOUSE_BUTTON_LEFT) && m_HasSelection) {
                HexCoords startHex = (move->isMoving && move->currentPathIndex < move->currentPath.size())
                                         ? move->currentPath[move->currentPathIndex]
                                         : HexMath::PixelToHex({pPos.x, pPos.y});

                std::vector<HexCoords> path = m_Grid.FindPath(startHex, m_SelectedHex);
                if (!path.empty()) {
                    m_PathTo = m_SelectedHex;
                    m_HasPathTo = true;
                    MovementSystem::SetPath(entity, std::move(path));
                }
            }

            if (!move->isMoving && m_HasPathTo) m_HasPathTo = false;
        }
    );

    // NPC
    m_Registry.ForEach<MovementComponent, TransformComponent, MaterialComponent>(
        [&](Entity entity, MovementComponent *move, TransformComponent *trans, MaterialComponent *mat) {
            if (entity.HasComponent<PlayerInputComponent>()) return;

            move->timePerStep = m_PlayerSpeed;

            if (!move->isMoving && !m_Grid.tiles.empty()) {
                HexCoords target;
                bool validTargetFound = false;
                int maxAttempts = 10;

                HexCoords playerHex{0, 0};
                m_Registry.ForEach<PlayerInputComponent, TransformComponent>(
                    [&](Entity e, PlayerInputComponent *p, TransformComponent *pt) {
                        playerHex = HexMath::PixelToHex({pt->transform.GetPosition().x, pt->transform.GetPosition().y});
                    });

                while (!validTargetFound && maxAttempts-- > 0) {
                    auto it = std::ranges::next(m_Grid.tiles.begin(), std::rand() % m_Grid.tiles.size());
                    target = it->first;
                    if (target != playerHex) validTargetFound = true;
                }

                if (validTargetFound) {
                    glm::vec3 npcPos = trans->transform.GetPosition();
                    std::vector<HexCoords> npcPath = m_Grid.FindPath(HexMath::PixelToHex({npcPos.x, npcPos.y}), target);
                    if (!npcPath.empty()) MovementSystem::SetPath(entity, npcPath);
                }
            }

            if (move->isMoving && move->currentPathIndex < move->currentPath.size()) {
                glm::vec2 nPos = glm::vec2(trans->transform.GetPosition());
                glm::vec2 targetDirW = HexMath::HexToWorld(move->currentPath[move->currentPathIndex]) - nPos;

                if (glm::length(targetDirW) > 0.01f) {
                    float degrees = glm::degrees(glm::atan(targetDirW.y, targetDirW.x));
                    if (degrees < 0.0f) degrees += 360.0f;
                    mat->material->texture = g_PlayerTextures[static_cast<int>(std::lround(degrees / 60.0f)) % 6];
                }
            }
        }
    );

    InputSystem::Update(m_Registry, *m_InputManager);
    MovementSystem::Update(m_Registry, dt, m_Grid);
}

void Scene::Render(Renderer &renderer) {
    for (const auto &[pos, tile]: m_Grid.tiles) {
        const Material &mat = (tile.type == TileType::Grass) ? g_GrassMat : g_SandMat;
        glm::vec2 worldPos = m_Grid.GetWorldPos(pos);
        Transform t;
        t.SetPosition({worldPos.x, worldPos.y, 0.0f});
        renderer.Submit(*g_HexMesh, mat, t);
    }

    if (m_HasSelection) {
        glm::vec2 worldPos = m_Grid.GetWorldPos(m_SelectedHex);
        Transform t;
        t.SetPosition({worldPos.x, worldPos.y, 0.01f});
        t.SetScale({1.08f, 1.08f, 1.0f});
        renderer.Submit(*g_HexMesh, g_OutlineMat, t);
    }

    if (m_HasPathTo) {
        glm::vec2 worldPos = m_Grid.GetWorldPos(m_PathTo);
        Transform t;
        t.SetPosition({worldPos.x, worldPos.y, 0.01f});
        t.SetScale({1.08f, 1.08f, 1.0f});
        renderer.Submit(*g_HexMesh, g_PathToMat, t);
    }

    if (m_Camera != nullptr) {
        m_Registry.ForEach<TransformComponent>([&](Entity entity, TransformComponent *transComp) {
            glm::vec3 pos = transComp->transform.GetPosition();
            glm::vec3 scale = transComp->transform.GetScale();
            transComp->transform.SetCustomMatrix(GetBillboardMatrix(pos, scale.x, scale.y, *m_Camera));
        });
    }

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
