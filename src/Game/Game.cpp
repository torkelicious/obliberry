#include "Game.h"
#include "Core/Constants.h"
#include "Core/ResourceManager.h"
#include "Renderer/MeshFactory.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <array>
#include <span>
#include <ranges>
#include "imgui.h"
#include "ECS/ECS.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/Movement.h"
#include "ECS/Systems/PlayerInput.h"
#include "ECS/Systems/RenderSystem.h"
#include "Map/MapSerialization.h"
#include "Renderer/GLDebug.h"

namespace {
    Mesh *g_HexMesh = nullptr;
    Shader *g_Shader = nullptr;

    std::array<Texture *, 6> g_PlayerTextures{};
    int g_PlayerDirection = 0;

    Texture *g_GrassTex = nullptr;
    Texture *g_SandTex = nullptr;
    Material g_GrassMat;
    Material g_SandMat;
    Material g_OutlineMat;
    Material g_PathToMat;
    HexCoords g_SelectedHex;
    HexCoords g_PathTo;
    bool g_HasSelection = false;
    bool g_HasPathTo = false;

    int g_GridSize = 20;
    int g_SandDensity = 50;
    float g_PlayerSpeed = 0.15f;

    bool savedOk = false;
    bool hasTestedSave = false;
    bool loadOk = false;
    bool hasTestedLoad = false;

    void InitMaterials() {
        g_GrassMat = {g_Shader, g_GrassTex, {1.0f, 1.0f, 1.0f, 1.f}};
        g_SandMat = {g_Shader, g_SandTex, {1.0f, 1.0f, 1.0f, 1.0f}};
        g_OutlineMat = {g_Shader, nullptr, {1.0f, 0.0f, 0.0f, 0.5f}};
        g_PathToMat = {g_Shader, nullptr, {1.0f, 1.0f, 1.0f, 0.5f}};
    }

    inline HexCoords GetPlayerHex(Game *game) {
        auto *trans = game->m_player.GetComponent<TransformComponent>();
        if (!trans) return {0, 0};
        glm::vec3 pos = trans->transform.GetPosition();
        return HexMath::PixelToHex({pos.x, pos.y});
    }
}


bool Game::TestFileWrite(const HexGrid &grid) const {
    std::string path = PathUtils::Join(MAP_PATH, "test", MAP_FILE_EXTENSION);
    size_t expectedBytes = MapIO::CalculateExpectedFileSize(grid.tiles.size());
    std::cout << "Expected Filesize is: " << expectedBytes << "\n";

    MapIO::SaveMap(path, grid);
    size_t actualBytes = std::filesystem::file_size(path);

    if (actualBytes == expectedBytes) {
        std::cout << "\nFile size matches expected size of " << expectedBytes << " bytes :)\n";
        return true;
    } else {
        std::cerr << "Expected: " << expectedBytes << " bytes, but file is " << actualBytes << " bytes... >:(\n";
        return false;
    }
}

bool Game::TestFileLoad(HexGrid &grid) const {
    std::string path = PathUtils::Join(MAP_PATH, "test", MAP_FILE_EXTENSION);
    std::cout << "Attempting to load map from " << path << "\n";

    if (MapIO::LoadMap(path, grid)) {
        std::cout << "Map loaded successfully with: " << grid.tiles.size() << " tiles B)\n";
        return true;
    } else {
        std::cerr << "\nFailed to load map file :(";
        return false;
    }
}

void Game::Start() {
    g_PlayerTextures[0] = ResourceManager::Load<
        Texture>("p_east", PathUtils::Join(TEXTURE_PATH, "player/player_e.png"));
    g_PlayerTextures[1] = ResourceManager::Load<Texture>("p_north_east",
                                                         PathUtils::Join(TEXTURE_PATH, "player/player_ne.png"));
    g_PlayerTextures[2] = ResourceManager::Load<Texture>("p_north_west",
                                                         PathUtils::Join(TEXTURE_PATH, "player/player_nw.png"));
    g_PlayerTextures[3] = ResourceManager::Load<
        Texture>("p_west", PathUtils::Join(TEXTURE_PATH, "player/player_w.png"));
    g_PlayerTextures[4] = ResourceManager::Load<Texture>("p_south_west",
                                                         PathUtils::Join(TEXTURE_PATH, "player/player_sw.png"));
    g_PlayerTextures[5] = ResourceManager::Load<Texture>("p_south_east",
                                                         PathUtils::Join(TEXTURE_PATH, "player/player_se.png"));

    g_Shader = ResourceManager::Load<Shader>(
        "base_shader",
        PathUtils::Join(SHADER_PATH, "base.vert"),
        PathUtils::Join(SHADER_PATH, "base.frag")
    );

    g_GrassTex = ResourceManager::Load<Texture>("grass_tex", PathUtils::Join(TEXTURE_PATH, "HexGrass.png"));
    g_SandTex = ResourceManager::Load<Texture>("sand_tex", PathUtils::Join(TEXTURE_PATH, "HexSand.png"));
    ResourceManager::Load<Texture>("player_tex", PathUtils::Join(TEXTURE_PATH, "snkl_berry.png"));

    Mesh *playerMesh = ResourceManager::LoadFromFactory<Mesh>("p_mesh", [] {
        auto data = MeshFactory::CreateStandingQuad(0.5f, 1);
        return std::make_unique<Mesh>(data.vertices, data.indices);
    });

    g_HexMesh = ResourceManager::LoadFromFactory<Mesh>("hex_mesh", [] {
        auto data = MeshFactory::CreatePointTopHex(0.5f);
        return std::make_unique<Mesh>(data.vertices, data.indices);
    });

    InitMaterials();
    GenerateTiles(g_Grid, g_GridSize);

    // create player
    m_player = Entity(m_Registry.CreateEntity(), &m_Registry);
    m_player.AddComponent<MeshComponent>(MeshComponent{playerMesh});
    m_player.AddComponent<MaterialComponent>(MaterialComponent{Material{g_Shader, g_PlayerTextures[0]}});
    m_player.AddComponent<TransformComponent>(TransformComponent{});
    m_player.AddComponent<MovementComponent>(MovementComponent{});
    m_player.AddComponent<PlayerInputComponent>(PlayerInputComponent{});

    m_ActiveEntities.push_back(m_player);

    auto *pTrans = m_player.GetComponent<TransformComponent>();
    if (pTrans) {
        pTrans->transform.SetPosition({0.0f, 0.0f, 0.05f});
        pTrans->transform.SetRotation({glm::radians(0.0f), 0.0f, 0.0f});
        pTrans->transform.SetScale({1.0f, 1.0f, 1.0f});
    }

    // create NPC
    m_NPC = Entity(m_Registry.CreateEntity(), &m_Registry);
    m_NPC.AddComponent<MeshComponent>(MeshComponent{playerMesh});
    m_NPC.AddComponent<MaterialComponent>(MaterialComponent{Material{g_Shader, g_PlayerTextures[0], {1, 0.5, 0.5, 1}}});
    m_NPC.AddComponent<TransformComponent>(TransformComponent{});
    m_NPC.AddComponent<MovementComponent>(MovementComponent{});

    m_ActiveEntities.push_back(m_NPC);

    auto *npcTrans = m_NPC.GetComponent<TransformComponent>();
    if (npcTrans) {
        npcTrans->transform.SetPosition({2.0f, 2.0f, 0.05f});
        npcTrans->transform.SetRotation({glm::radians(0.0f), 0.0f, 0.0f});
        npcTrans->transform.SetScale({1.0f, 1.0f, 1.0f});
    }
}

void Game::Update(float dt) {
    // Performance Window
    constexpr float PADDING = 10.0f;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_pos;
    window_pos.x = (work_pos.x + work_size.x) - PADDING;
    window_pos.y = work_pos.y + PADDING;
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin("Performance", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.7f, 1.0f), "perf");
    ImGui::Separator();
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::End();

    // Main Control Window
    ImGui::Begin("obliberry");
    if (ImGui::CollapsingHeader("World Generation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        ImGui::SliderInt("Hex Radius", &g_GridSize, 2, 100, "%d tiles");
        ImGui::SliderInt("Sand Density", &g_SandDensity, 0, 100, "%d%%");
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.5f, 1.0f));

        if (ImGui::Button("Regenerate Grid", ImVec2(-1, 0))) {
            MovePlayerToCenter(); // Avoid breaking pathfinding
            GenerateTiles(g_Grid, g_GridSize, g_SandDensity);
        }
        ImGui::PopStyleColor(2);

        ImVec4 colorSave = !hasTestedSave
                               ? ImVec4(0.4f, 0.4f, 0.4f, 1.0f)
                               : (savedOk ? ImVec4(0.0f, 0.6f, 0.0f, 1.0f) : ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, colorSave);
        if (ImGui::Button("Run file creation test", ImVec2(-1, 0))) {
            savedOk = TestFileWrite(g_Grid);
            hasTestedSave = true;
            MovePlayerToCenter();
        }
        ImGui::PopStyleColor(1);

        ImVec4 colorLoad = !hasTestedLoad
                               ? ImVec4(0.4f, 0.4f, 0.4f, 1.0f)
                               : (loadOk ? ImVec4(0.0f, 0.6f, 0.0f, 1.0f) : ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, colorLoad);
        if (ImGui::Button("Run file load test", ImVec2(-1, 0))) {
            loadOk = TestFileLoad(g_Grid);
            hasTestedLoad = true;
            MovePlayerToCenter();
        }
        ImGui::PopStyleColor(1);
    }
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Telemetry", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        ImGui::SliderFloat("Step Duration", &g_PlayerSpeed, 0.05f, 1.0f, "%.2fs per tile");
        ImGui::Separator();
        ImGui::Spacing();

        HexCoords pCoords = GetPlayerHex(this);
        ImGui::Text("Player Hex Pos:  [Q: %d, R: %d]", pCoords.q, pCoords.r);
        auto *moveComp = m_player.GetComponent<MovementComponent>();
        if (moveComp) {
            ImGui::Text("Movement Status: ");
            ImGui::SameLine();
            if (moveComp->isMoving) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Moving (Node %zu/%zu)", moveComp->currentPathIndex,
                                   moveComp->currentPath.size());
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Idle");
            }
        }
        ImGui::Spacing();
        ImGui::Text("Target Hex:      ");
        ImGui::SameLine();
        if (g_HasSelection) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "[Q: %d, R: %d]", g_SelectedHex.q, g_SelectedHex.r);
        } else {
            ImGui::TextDisabled("None");
        }
    }

    if (ImGui::CollapsingHeader("ECS Telemetry & Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {
        // registry stats
        ImGui::Text("Active Handle Count: %zu", m_ActiveEntities.size());
        ImGui::Separator();

        static int selectedEntityIdx = -1;

        // reset selection if entities drop out or array shrinks out of bounds
        if (selectedEntityIdx >= static_cast<int>(m_ActiveEntities.size())) {
            selectedEntityIdx = -1;
        }

        // layout configuration
        float availableWidth = ImGui::GetContentRegionAvail().x;
        // entity list
        ImGui::BeginChild("EntityListView", ImVec2(availableWidth * 0.4f, 220.0f), true);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.7f, 1.0f), "Entities");
        ImGui::Separator();

        for (size_t i = 0; i < m_ActiveEntities.size(); ++i) {
            const auto &entity = m_ActiveEntities[i];
            EntityID id = static_cast<EntityID>(entity);

            // descriptive labels
            std::string label = "ID: " + std::to_string(id);
            if (entity == m_player) label += " [Player]";
            else if (entity == m_NPC) label += " [NPC]";

            if (ImGui::Selectable(label.c_str(), selectedEntityIdx == static_cast<int>(i))) {
                selectedEntityIdx = static_cast<int>(i);
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // component inspector
        ImGui::BeginChild("ComponentInspectorView", ImVec2(0, 220.0f), true);

        if (selectedEntityIdx >= 0 && selectedEntityIdx < static_cast<int>(m_ActiveEntities.size())) {
            const auto &entity = m_ActiveEntities[selectedEntityIdx];

            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Components");
            ImGui::Separator();

            // TransformComponent Inspection
            if (auto *transComp = entity.GetComponent<TransformComponent>()) {
                if (ImGui::TreeNodeEx("Transform Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                    glm::vec3 pos = transComp->transform.GetPosition();
                    glm::vec3 rot = transComp->transform.GetRotation();
                    glm::vec3 scale = transComp->transform.GetScale();

                    ImGui::Text(" Pos:   %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
                    ImGui::Text(" Rot:   %.1f deg", glm::degrees(rot.x));
                    ImGui::Text(" Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
                    ImGui::TreePop();
                }
            }

            // MovementComponent Inspection
            if (auto *moveComp = entity.GetComponent<MovementComponent>()) {
                if (ImGui::TreeNodeEx("Movement Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Text(" State: %s", moveComp->isMoving ? "Moving" : "Idle");
                    ImGui::Text(" Node:  %zu / %zu", moveComp->currentPathIndex, moveComp->currentPath.size());
                    ImGui::Text(" Timer: %.2fs / %.2fs", moveComp->stepTimer, moveComp->timePerStep);
                    ImGui::TreePop();
                }
            }

            // MaterialComponent Inspection
            if (auto *matComp = entity.GetComponent<MaterialComponent>()) {
                if (ImGui::TreeNodeEx("Material Component")) {
                    ImGui::Text(" Shader:  %p", static_cast<void *>(matComp->material.shader));
                    ImGui::Text(" Texture: %p", static_cast<void *>(matComp->material.texture));

                    // dynamic color modification via UI
                    ImGui::ColorEdit4("Tint", &matComp->material.color.r, ImGuiColorEditFlags_NoInputs);
                    ImGui::TreePop();
                }
            }

            // MeshComponent Inspection
            if (auto *meshComp = entity.GetComponent<MeshComponent>()) {
                if (ImGui::TreeNodeEx("Mesh Component")) {
                    ImGui::Text(" Mesh Data Ref: %p", static_cast<void *>(meshComp->mesh));
                    ImGui::TreePop();
                }
            }

            // PlayerInputComponent Inspection
            if (auto *inputComp = entity.GetComponent<PlayerInputComponent>()) {
                if (ImGui::TreeNodeEx("Player Input Component")) {
                    ImGui::Text(" Click Bindings: Left(%d), Right(%d)", inputComp->LeftClick, inputComp->RightClick);
                    ImGui::Text(" Vector Bindings: W, A, S, D");
                    ImGui::TreePop();
                }
            }
        } else {
            ImGui::TextDisabled("Select an entity :)");
        }

        ImGui::EndChild();
    }

    ImGui::End();

    if (auto *pMove = m_player.GetComponent<MovementComponent>()) {
        pMove->timePerStep = g_PlayerSpeed;
    }

    float windowWidth = static_cast<float>(m_Window->GetWidth());
    float windowHeight = static_cast<float>(m_Window->GetHeight());

    // --- Camera Zoom & Pan ---
    if (m_InputManager->scrollY != 0.0) {
        m_Camera->Zoom += static_cast<float>(m_InputManager->scrollY) * ZOOM_SPEED;
        if (m_Camera->Zoom < 0.5f) m_Camera->Zoom = 0.5f;
        if (m_Camera->Zoom > 5.0f) m_Camera->Zoom = 5.0f;
    }

    float edgeMargin = 20.0f;
    glm::vec2 screenPan(0.0f);
    HexMath::Point mousePos = {
        static_cast<float>(m_InputManager->mousePosX),
        static_cast<float>(m_InputManager->mousePosY)
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

    // mouse & Player Rotation
    glm::vec2 worldPos = m_Camera->MouseToWorld(mousePos.x, mousePos.y, windowWidth, windowHeight);
    auto *pTrans = m_player.GetComponent<TransformComponent>();
    glm::vec2 pPos = pTrans ? glm::vec2(pTrans->transform.GetPosition()) : glm::vec2(0.0f);
    auto *moveComp = m_player.GetComponent<MovementComponent>();

    glm::vec2 targetDir;
    bool hasTarget = false;

    if (moveComp && moveComp->isMoving && moveComp->currentPathIndex < moveComp->currentPath.size()) {
        HexCoords nextHex = moveComp->currentPath[moveComp->currentPathIndex];
        targetDir = HexMath::HexToWorld(nextHex) - pPos;
        hasTarget = true;
    } else {
        targetDir = worldPos - pPos;
        if (glm::length(targetDir) > (HEX_SIZE - 0.005f)) {
            hasTarget = true;
        }
    }

    if (hasTarget) {
        float radians = glm::atan(targetDir.y, targetDir.x);
        float degrees = glm::degrees(radians);

        if (degrees < 0.0f) degrees += 360.0f;

        g_PlayerDirection = static_cast<int>(std::lround(degrees / 60.0f)) % 6;

        if (auto *matComp = m_player.GetComponent<MaterialComponent>()) {
            matComp->material.texture = g_PlayerTextures[g_PlayerDirection];
        }
    }

    // tile selection & pathing
    HexCoords hexPosOnMpos = HexMath::PixelToHex({worldPos.x, worldPos.y});
    if (Tile *tileAtMouse = g_Grid.Get(hexPosOnMpos); !tileAtMouse) {
        g_HasSelection = false;
    } else {
        g_SelectedHex = hexPosOnMpos;
        g_HasSelection = true;
    }

    if (m_InputManager->IsMousePressed(GLFW_MOUSE_BUTTON_LEFT) && g_HasSelection) {
        HexCoords startHex;
        if (moveComp && moveComp->isMoving && moveComp->currentPathIndex < moveComp->currentPath.size()) {
            startHex = moveComp->currentPath[moveComp->currentPathIndex];
        } else {
            startHex = GetPlayerHex(this);
        }

        std::vector<HexCoords> path = g_Grid.FindPath(startHex, g_SelectedHex);
        if (!path.empty()) {
            g_PathTo = g_SelectedHex;
            g_HasPathTo = true;
            MovementSystem::SetPath(m_player, path);
        }
    }

    if (moveComp && !moveComp->isMoving && g_HasPathTo) {
        g_HasPathTo = false;
    }

    // funny npc wandering
    auto *npcMove = m_NPC.GetComponent<MovementComponent>();
    auto *npcTrans = m_NPC.GetComponent<TransformComponent>();

    if (npcMove) {
        npcMove->timePerStep = g_PlayerSpeed;

        if (!npcMove->isMoving && !g_Grid.tiles.empty() && npcTrans) {
            HexCoords target;
            bool validTargetFound = false;
            int maxAttempts = 10;

            while (!validTargetFound && maxAttempts-- > 0) {
                auto it = std::ranges::next(g_Grid.tiles.begin(), std::rand() % g_Grid.tiles.size());
                target = it->first;

                if (target != GetPlayerHex(this)) {
                    validTargetFound = true;
                }
            }

            if (validTargetFound) {
                glm::vec3 npcPos = npcTrans->transform.GetPosition();
                HexCoords start = HexMath::PixelToHex({npcPos.x, npcPos.y});
                std::vector<HexCoords> npcPath = g_Grid.FindPath(start, target);

                if (!npcPath.empty()) {
                    MovementSystem::SetPath(m_NPC, npcPath);
                }
            }
        }

        if (npcMove->isMoving && npcTrans && npcMove->currentPathIndex < npcMove->currentPath.size()) {
            glm::vec2 nPos = glm::vec2(npcTrans->transform.GetPosition());
            HexCoords nextHex = npcMove->currentPath[npcMove->currentPathIndex];
            glm::vec2 targetDirW = HexMath::HexToWorld(nextHex) - nPos;

            if (glm::length(targetDirW) > 0.01f) {
                float radians = glm::atan(targetDirW.y, targetDirW.x);
                float degrees = glm::degrees(radians);
                if (degrees < 0.0f) degrees += 360.0f;

                int npcDirection = static_cast<int>(std::lround(degrees / 60.0f)) % 6;

                if (auto *matComp = m_NPC.GetComponent<MaterialComponent>()) {
                    matComp->material.texture = g_PlayerTextures[npcDirection];
                }
            }
        }
    }

    // Systems now take Entities directly! No Registry required.
    InputSystem::Update(m_ActiveEntities, *m_InputManager);
    MovementSystem::Update(m_ActiveEntities, dt, g_Grid);
}

void Game::Render(Renderer &renderer) {
    if (m_Camera != nullptr) {
        float windowWidth = static_cast<float>(m_Window->GetWidth());
        float windowHeight = static_cast<float>(m_Window->GetHeight());
        renderer.SetCamera(*m_Camera, windowWidth, windowHeight);
    }

    renderer.BeginFrame();

    for (const auto &[pos, tile]: g_Grid.tiles) {
        const Material &mat = (tile.type == TileType::Grass) ? g_GrassMat : g_SandMat;
        glm::vec2 worldPos = g_Grid.GetWorldPos(pos);
        Transform t;
        t.SetPosition({worldPos.x, worldPos.y, 0.0f});
        renderer.Submit(*g_HexMesh, mat, t);
    }

    if (g_HasSelection) {
        glm::vec2 worldPos = g_Grid.GetWorldPos(g_SelectedHex);
        Transform t;
        t.SetPosition({worldPos.x, worldPos.y, 0.01f});
        t.SetScale({1.08f, 1.08f, 1.0f});
        renderer.Submit(*g_HexMesh, g_OutlineMat, t);
    }

    if (g_HasPathTo) {
        glm::vec2 worldPos = g_Grid.GetWorldPos(g_PathTo);
        Transform t;
        t.SetPosition({worldPos.x, worldPos.y, 0.01f});
        t.SetScale({1.08f, 1.08f, 1.0f});
        renderer.Submit(*g_HexMesh, g_PathToMat, t);
    }

    RenderSystem::Render(m_ActiveEntities, renderer);

    renderer.Flush();
}

void Game::GenerateTiles(HexGrid &map, int size, const int percent) {
    if (!map.tiles.empty()) {
        map.tiles.clear();
    }

    size = size / 2;
    for (int q = -size; q < size; q++) {
        for (int r = -size; r < size; r++) {
            HexCoords pos{q, r};
            TileType type = (std::rand() % 100 < percent) ? TileType::Sand : TileType::Grass;
            map.EmplaceTile(pos, type);
        }
    }
}

void Game::MovePlayerToCenter() {
    auto *transComp = m_player.GetComponent<TransformComponent>();
    auto *moveComp = m_player.GetComponent<MovementComponent>();
    MovementSystem::CancelPath(moveComp);

    if (transComp) {
        glm::vec3 pos = transComp->transform.GetPosition();
        pos.x = 0.0f;
        pos.y = 0.0f;
        transComp->transform.SetPosition(pos);
    }
}

void Game::Shutdown() const {
    ResourceManager::Shutdown();
}
