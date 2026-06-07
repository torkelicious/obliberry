#include "Game.h"
#include "Core/Constants.h"
#include "Core/ResourceManager.h"
#include "Renderer/MeshFactory.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include "imgui.h"
#include "ECS/ECS.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/Movement.h"
#include "ECS/Systems/PlayerInput.h"
#include "Map/MapSerialization.h"

// Globals for game
namespace {
    Entity *m_player = ResourceManager::Load<Entity>("player");
    Mesh *g_HexMesh = nullptr;
    Shader *g_Shader = nullptr;
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

    void InitMaterials() {
        g_GrassMat = {g_Shader, g_GrassTex, {1.0f, 1.0f, 1.0f, 1.f}};
        g_SandMat = {g_Shader, g_SandTex, {1.0f, 1.0f, 1.0f, 1.0f}};
        // red outline material
        g_OutlineMat = {g_Shader, nullptr, {1.0f, 0.0f, 0.0f, 0.5f}};
        // selected hex when pathing
        g_PathToMat = {g_Shader, nullptr, {1.0f, 1.0f, 1.0f, 0.5f}};
    }

    inline HexCoords GetPlayerHex() {
        if (!m_player || !m_player->HasComponent<TransformComponent>()) return {0, 0};
        glm::vec3 pos = m_player->GetComponent<TransformComponent>()->transform.GetPosition();
        return HexMath::PixelToHex({pos.x, pos.y});
    }
}

// temp test
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
        std::cerr << "Expected: " << expectedBytes <<
                " bytes, but file is " << actualBytes << " bytes... >:(\n";
        return false;
    }
}

bool Game::TestFileLoad(HexGrid &grid) const {
    std::string path = PathUtils::Join(MAP_PATH, "test", MAP_FILE_EXTENSION);
    std::cout << "Attempting to load map from " << path << "\n";
    if (MapIO::LoadMap(path, grid)) {
        std::cout << "map loaded successfully with: " << grid.tiles.size() << " tiles B)\n";
        return true;
    } else {
        std::cerr << "\n failed to load map file :(";
        return false;
    }
}


// test

void Game::Start() {
    g_Shader = ResourceManager::Load<Shader>(
        "base_shader",
        PathUtils::Join(SHADER_PATH, "base.vert"),
        PathUtils::Join(SHADER_PATH, "base.frag")
    );

    g_GrassTex = ResourceManager::Load<Texture>(
        "grass_tex",
        PathUtils::Join(TEXTURE_PATH, "HexGrass.png")
    );

    g_SandTex = ResourceManager::Load<Texture>(
        "sand_tex",
        PathUtils::Join(TEXTURE_PATH, "HexSand.png")
    );

    Texture *ptex = ResourceManager::Load<Texture>(
        "player_tex",
        PathUtils::Join(TEXTURE_PATH, "uv_grid.jpg")
    );

    Mesh *playerMesh = ResourceManager::LoadFromFactory<Mesh>(
        "p_mesh",
        [] {
            auto data = MeshFactory::CreateQuad();
            return std::make_unique<Mesh>(data.vertices, data.indices);
        }
    );

    g_HexMesh = ResourceManager::LoadFromFactory<Mesh>(
        "hex_mesh",
        [] {
            auto data = MeshFactory::CreatePointTopHex(0.5f);
            return std::make_unique<Mesh>(data.vertices, data.indices);
        }
    );

    InitMaterials();
    GenerateTiles(g_Grid, 20);

    m_player->AddComponent<MeshComponent>()->mesh = playerMesh;
    m_player->AddComponent<MaterialComponent>()->material = Material{g_Shader, ptex};
    m_player->AddComponent<TransformComponent>();

    auto *pTrans = m_player->GetComponent<TransformComponent>();
    pTrans->transform.SetPosition({0.0f, 0.0f, 0.05f});
    pTrans->transform.SetRotation({glm::radians(0.0f), 0.0f, 0.0f});
    pTrans->transform.SetScale({1.0f, 1.0f, 1.0f});

    m_player->AddComponent<PlayerInput>();
    m_player->AddComponent<MovementComponent>();
}

bool savedOk = false;
static bool hasTestedSave = false;
bool loadOk = false;
static bool hasTestedLoad = false;


int i = 20;
int p = 50;
float playerSpeed = 0.15f;

void Game::Update(float dt) {
    // game gui

    // performance
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
                 ImGuiWindowFlags_NoDecoration |
                 ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoFocusOnAppearing |
                 ImGuiWindowFlags_NoNav |
                 ImGuiWindowFlags_NoMove);

    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.7f, 1.0f), "perf");
    ImGui::Separator();
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Begin("obliberry"); // Create an imgui window
    if (ImGui::CollapsingHeader("World Generation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        ImGui::SliderInt("Hex Radius", &i, 2, 100, "%d tiles");
        ImGui::SliderInt("Sand Density", &p, 0, 100, "%d%%");
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.5f, 1.0f));

        if (ImGui::Button("Regenerate Grid", ImVec2(-1, 0))) {
            MovePlayerToCenter(); // Avoid breaking pathfinding
            GenerateTiles(g_Grid, i, p);
        }
        ImGui::PopStyleColor(2);

        // file save test
        ImVec4 color;
        if (!hasTestedSave) {
            color = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        } else {
            color = savedOk ? ImVec4(0.0f, 0.6f, 0.0f, 1.0f) : ImVec4(0.6f, 0.0f, 0.0f, 1.0f);
        }
        ImGui::PushStyleColor(ImGuiCol_Button, color);
        if (ImGui::Button("Run file creation test", ImVec2(-1, 0))) {
            savedOk = TestFileWrite(g_Grid);
            hasTestedSave = true;
            MovePlayerToCenter();
        }
        ImGui::PopStyleColor(1);
        // file save test

        // file load test
        ImVec4 color2;
        if (!hasTestedLoad) {
            color2 = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        } else {
            color2 = loadOk ? ImVec4(0.0f, 0.6f, 0.0f, 1.0f) : ImVec4(0.6f, 0.0f, 0.0f, 1.0f);
        }
        ImGui::PushStyleColor(ImGuiCol_Button, color2);
        if (ImGui::Button("Run file load test", ImVec2(-1, 0))) {
            loadOk = TestFileLoad(g_Grid);
            hasTestedLoad = true;
            MovePlayerToCenter();
        }
        ImGui::PopStyleColor(1);
        // file load test
    }
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Telemetry", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        ImGui::SliderFloat("Step Duration", &playerSpeed, 0.05f, 1.0f, "%.2fs per tile");
        ImGui::Separator();
        ImGui::Spacing();

        // telemetry
        HexCoords pCoords = GetPlayerHex();
        ImGui::Text("Player Hex Pos:  [Q: %d, R: %d]", pCoords.q, pCoords.r);
        auto *moveComp = m_player->GetComponent<MovementComponent>();
        if (moveComp) {
            ImGui::Text("Movement Status: ");
            ImGui::SameLine();
            if (moveComp->isMoving) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Moving (Node %zu/%zu)",
                                   moveComp->currentPathIndex, moveComp->currentPath.size());
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
    ImGui::End();
    // game gui

    // apply slider change
    m_player->GetComponent<MovementComponent>()->timePerStep = playerSpeed;

    MovementSystem::Update(dt, *m_player, g_Grid);
    /*
        // ReSharper disable once CppDFAConstantConditions
        if (m_Camera == nullptr) { // idk why the ide claims this is always returning when it isnt, so im commenting it out even though i shouldnt
            return;
        }*/
    float windowWidth = static_cast<float>(m_Window->GetWidth());
    float windowHeight = static_cast<float>(m_Window->GetHeight());

    // zoom
    if (m_InputManager->scrollY != 0.0) {
        m_Camera->Zoom += static_cast<float>(m_InputManager->scrollY) * ZOOM_SPEED;

        // clamp the zoom to prevent the view from inverting or zooming out infinitely
        if (m_Camera->Zoom < 0.5f) m_Camera->Zoom = 0.5f;
        if (m_Camera->Zoom > 5.0f) m_Camera->Zoom = 5.0f;
    }

    float edgeMargin = 20.0f; // Boundary size in pixels
    glm::vec2 screenPan(0.0f);

    HexMath::Point mousePos = {
        static_cast<float>(m_InputManager->mousePosX),
        static_cast<float>(m_InputManager->mousePosY)
    };

    // Check X axis edges
    if (mousePos.x <= edgeMargin) {
        screenPan.x -= 1.0f; // Pan Left
    } else if (mousePos.x >= windowWidth - edgeMargin) {
        screenPan.x += 1.0f; // Pan Right
    }

    // Check Y axis edges
    if (mousePos.y <= edgeMargin) {
        screenPan.y += 1.0f; // Pan Up
    } else if (mousePos.y >= windowHeight - edgeMargin) {
        screenPan.y -= 1.0f; // Pan Down
    }

    if (glm::length(screenPan) > 0.0f) {
        screenPan = glm::normalize(screenPan);
        glm::mat4 invRot = glm::inverse(m_Camera->GetRotation());
        glm::vec4 worldPan = invRot * glm::vec4(screenPan.x, screenPan.y, 0.0f, 0.0f);
        m_Camera->Position += glm::vec2(worldPan.x, worldPan.y) * PAN_SPEED * dt * (1.0f / m_Camera->Zoom);
    }

    // mouse selection tile stuff
    glm::vec2 worldPos = m_Camera->MouseToWorld(mousePos.x, mousePos.y, windowWidth, windowHeight);
    HexCoords hexPosOnMpos = HexMath::PixelToHex({worldPos.x, worldPos.y});

    Tile *tileAtMouse = g_Grid.Get(hexPosOnMpos);

    if (!tileAtMouse) {
        g_HasSelection = false;
    } else {
        g_SelectedHex = hexPosOnMpos;
        g_HasSelection = true;
    }

    auto *moveComp = m_player->GetComponent<MovementComponent>();

    // click to move
    if (m_InputManager->IsMousePressed(GLFW_MOUSE_BUTTON_LEFT) && g_HasSelection) {
        HexCoords startHex;

        if (moveComp && moveComp->isMoving && moveComp->currentPathIndex < moveComp->currentPath.size()) {
            startHex = moveComp->currentPath[moveComp->currentPathIndex];
        } else {
            startHex = GetPlayerHex();
        }

        std::vector<HexCoords> path = g_Grid.FindPath(startHex, g_SelectedHex);

        if (!path.empty()) {
            g_PathTo = g_SelectedHex;
            g_HasPathTo = true;
            MovementSystem::SetPath(*m_player, path);
        }
    }

    if (!moveComp->isMoving && g_HasPathTo) {
        g_HasPathTo = false;
    }
}


void Game::Render(Renderer &renderer) {
    if (m_Camera != nullptr) {
        float windowWidth = static_cast<float>(m_Window->GetWidth());
        float windowHeight = static_cast<float>(m_Window->GetHeight());
        renderer.SetCamera(*m_Camera, windowWidth, windowHeight); // implement proper method for this later
    }

    renderer.BeginFrame();

    // draw tiles
    for (const auto &[pos, tile]: g_Grid.tiles) {
        const Material &mat =
                (tile.type == TileType::Grass)
                    ? g_GrassMat
                    : g_SandMat;
        glm::vec2 worldPos = g_Grid.GetWorldPos(pos);
        Transform t;
        t.SetPosition({worldPos.x, worldPos.y, 0.0f});
        renderer.Submit(*g_HexMesh, mat, t);
    }

    // draw selection on top
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

    // draw player
    if (m_player->HasComponent<MeshComponent>() && m_player->HasComponent<MaterialComponent>()) {
        Mesh *pmesh = m_player->GetComponent<MeshComponent>()->mesh;
        Material &pMat = m_player->GetComponent<MaterialComponent>()->material;
        Transform pt = m_player->GetComponent<TransformComponent>()->transform;
        renderer.Submit(*pmesh, pMat, pt);
    }

    renderer.Flush();
}

// temp testing ig
void Game::GenerateTiles(HexGrid &map, int size, const int percent) {
    // generate a size x size area centered around origin
    // regenerate
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
    auto *transComp = m_player->GetComponent<TransformComponent>();
    auto *moveComp = m_player->GetComponent<MovementComponent>();
    moveComp->Cancel();
    glm::vec3 pos = transComp->transform.GetPosition();
    pos.x = 0.0f;
    pos.y = 0.0f;
    transComp->transform.SetPosition(pos);
}


void Game::Shutdown() const {
    ResourceManager::Shutdown();
}
