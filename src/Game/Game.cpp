#include "Game.h"
#include "Core/Constants.h"
#include "../IO/MapSerialization.h"
#include "imgui.h"
#include <filesystem>
#include <random>
#include "../IO/AssetLoader.h"
#include "Core/Utils.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/PlayerInputComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Systems/MovementSystem.h"
#include "IO/EntityFactory.h"
#include "Renderer/MeshFactory.h"

namespace {
    int g_GridSize = 20;
    int g_SandDensity = 50;
    bool savedOk = false;
    bool hasTestedSave = false;
    bool loadOk = false;
    bool hasTestedLoad = false;
    float g_PlayerSpeed = 0.15f; // local state for UI
}

bool Game::TestFileWrite(const HexGrid &grid) const {
    std::string path = PathUtils::Join(MAP_PATH, "test", MAP_FILE_EXTENSION);
    size_t expectedBytes = MapIO::CalculateExpectedFileSize(grid.tiles.size());
    MapIO::Serialize(path, grid);
    size_t actualBytes = std::filesystem::file_size(path);
    return (actualBytes == expectedBytes);
}

bool Game::TestFileLoad(HexGrid &grid) const {
    std::string path = PathUtils::Join(MAP_PATH, "test", MAP_FILE_EXTENSION);
    return MapIO::Deserialize(path, grid);
}

void Game::Start() {
    AssetLoader::RegisterMeshFactory(
        "Quad",
        []() -> std::shared_ptr<Mesh> {
            auto data = MeshFactory::CreateQuad();
            return std::make_shared<Mesh>(data.vertices, data.indices);
        }
    );

    AssetLoader::RegisterMeshFactory(
        "PointTopHex",
        []() -> std::shared_ptr<Mesh> {
            auto data = MeshFactory::CreatePointTopHex(0.5f);
            return std::make_shared<Mesh>(data.vertices, data.indices);
        }
    );

    auto initialScene = std::make_unique<Scene>(m_Context);
    m_SceneManager.LoadScene(std::move(initialScene));
}

void Game::Update(float dt) {
    m_Context.deltaTime = dt;

    DrawInterface();
    if (m_CurrentState == GameState::Gameplay) {
        m_SceneManager.Update(dt);
    }
}

void Game::Render(Renderer &renderer) {
    if (m_Context.camera != nullptr) {
        renderer.SetCamera(*m_Context.camera, m_Context.window->GetWidth(), m_Context.window->GetHeight());
    }
    renderer.BeginFrame();
    m_SceneManager.Render(renderer);
    renderer.Flush();
}

void Game::DrawInterface() {
    constexpr float PADDING = 10.0f;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();

    // PERFORMANCE
    ImGui::SetNextWindowPos(
        ImVec2((viewport->WorkPos.x + viewport->WorkSize.x) - PADDING, viewport->WorkPos.y + PADDING), ImGuiCond_Always,
        ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin("Performance", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.7f, 1.0f), "perf");
    ImGui::Separator();
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Delta Time: %.4f ms", m_Context.deltaTime * 1000.0f);
    ImGui::End();

    Scene *activeScene = m_SceneManager.GetCurrentScene();
    if (!activeScene) return;

    Registry &reg = activeScene->GetRegistry();

    // map entity components for the UI
    MapComponent *mapComp = nullptr;
    MapStateComponent *stateComp = nullptr;
    reg.ForEach<MapComponent, MapStateComponent>([&](Entity, MapComponent *m, MapStateComponent *s) {
        mapComp = m;
        stateComp = s;
    });

    auto resetEntitiesToCenter = [&reg]() {
        reg.ForEach<MovementComponent>([&](Entity e, MovementComponent *) {
            MovementSystem::MoveToCenter(e);
        });
    };

    ImGui::Begin("obliberry");

    ImGui::Text("State:");
    ImGui::SameLine();
    if (m_CurrentState == GameState::Gameplay) ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Gameplay");
    else if (m_CurrentState == GameState::Paused) ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Paused");

    if (ImGui::Button(m_CurrentState == GameState::Gameplay ? "Pause" : "Resume", ImVec2(-1, 0))) {
        m_CurrentState = (m_CurrentState == GameState::Gameplay) ? GameState::Paused : GameState::Gameplay;
    }
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("World & Data", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (mapComp) {
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Active Grid Tiles: %zu", mapComp->grid.tiles.size());
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "File Binding: %s", mapComp->mapFilePath.c_str());
            ImGui::Separator();
        }

        ImGui::SliderInt("Hex Radius", &g_GridSize, 2, 100, "%d tiles");
        ImGui::SliderInt("Sand Density", &g_SandDensity, 0, 100, "%d%%");

        if (ImGui::Button("Regenerate Grid", ImVec2(-1, 0))) {
            if (mapComp) {
                mapComp->grid.tiles.clear();
                std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<int> dist(0, 99);
                for (int q = -g_GridSize; q < g_GridSize; q++) {
                    for (int r = -g_GridSize; r < g_GridSize; r++) {
                        mapComp->grid.EmplaceTile(
                            {q, r}, (dist(rng) < g_SandDensity) ? TileType::Sand : TileType::Grass);
                    }
                }
                resetEntitiesToCenter();
            }
        }

        ImGui::Spacing();
        ImGui::Text("I/O Tests:");
        ImVec4 colorSave = !hasTestedSave
                               ? ImVec4(0.4f, 0.4f, 0.4f, 1.0f)
                               : (savedOk ? ImVec4(0.0f, 0.6f, 0.0f, 1.0f) : ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, colorSave);
        if (ImGui::Button("Test Save Map", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 4.0f, 0))) {
            if (mapComp) {
                savedOk = TestFileWrite(mapComp->grid);
                hasTestedSave = true;
            }
        }
        ImGui::PopStyleColor(1);
        ImGui::SameLine();

        ImVec4 colorLoad = !hasTestedLoad
                               ? ImVec4(0.4f, 0.4f, 0.4f, 1.0f)
                               : (loadOk ? ImVec4(0.0f, 0.6f, 0.0f, 1.0f) : ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, colorLoad);
        if (ImGui::Button("Test Load Map", ImVec2(-1, 0))) {
            if (mapComp) {
                loadOk = TestFileLoad(mapComp->grid);
                hasTestedLoad = true;
                resetEntitiesToCenter();
            }
        }
        ImGui::PopStyleColor(1);
    }

    if (ImGui::CollapsingHeader("Camera & Input", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (m_Context.camera) {
            ImGui::Text("Camera Pos:  [%.2f, %.2f]", m_Context.camera->Position.x, m_Context.camera->Position.y);
            ImGui::Text("Camera Zoom: %.2fx", m_Context.camera->Zoom);
            ImGui::Separator();
        }

        ImGui::Text("Mouse:");
        if (stateComp && stateComp->hasSelection) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "  Hex: [Q: %d, R: %d]", stateComp->selectedHex.q,
                               stateComp->selectedHex.r);
            if (mapComp && mapComp->grid.HasTile(stateComp->selectedHex)) {
                TileType type = mapComp->grid.Get(stateComp->selectedHex)->type;
                ImGui::Text("  Type: %s", type == TileType::Grass ? "Grass" : "Sand");
            } else {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "  the void....");
            }
        } else {
            ImGui::TextDisabled("  the void....");
        }
    }
    if (ImGui::CollapsingHeader("ECS", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto &livingEntities = reg.GetLivingEntities();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Active Entities: %zu", livingEntities.size());
        ImGui::Separator();

        static int selectedEntityIdx = -1;
        if (selectedEntityIdx >= static_cast<int>(livingEntities.size())) selectedEntityIdx = -1;

        ImGui::BeginChild("EntityListView", ImVec2(ImGui::GetContentRegionAvail().x * 0.35f, 350.0f), true);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.7f, 1.0f), "Handles");
        ImGui::Separator();

        for (size_t i = 0; i < livingEntities.size(); ++i) {
            EntityID id = livingEntities[i];
            Entity entity(id, &reg);

            std::string label = "ID: " + std::to_string(id);
            if (entity.HasComponent<PlayerInputComponent>()) label += " [Player]";
            else if (entity.HasComponent<MovementComponent>()) label += " [NPC]";
            else if (entity.HasComponent<MapComponent>()) label += " [Map]";

            if (ImGui::Selectable(label.c_str(), selectedEntityIdx == static_cast<int>(i))) {
                selectedEntityIdx = static_cast<int>(i);
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("ComponentInspectorView", ImVec2(0, 350.0f), true);

        if (selectedEntityIdx >= 0 && selectedEntityIdx < static_cast<int>(livingEntities.size())) {
            Entity entity(livingEntities[selectedEntityIdx], &reg);

            if (ImGui::BeginTabBar("EntityTabs")) {
                if (ImGui::BeginTabItem("Components")) {
                    ImGui::Spacing();
                    if (auto *transComp = entity.GetComponent<TransformComponent>()) {
                        if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                            glm::vec3 pos = transComp->transform.GetPosition();
                            ImGui::Text("  World Pos: [%.2f, %.2f, %.2f]", pos.x, pos.y, pos.z);

                            HexCoords hCoords = HexMath::PixelToHex({pos.x, pos.y});
                            ImGui::Text("  Hex Space: [Q: %d, R: %d]", hCoords.q, hCoords.r);
                            ImGui::TreePop();
                        }
                    }
                    if (auto *moveComp = entity.GetComponent<MovementComponent>()) {
                        if (ImGui::TreeNodeEx("Pathfinding & Movement", ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::Text("  Status: ");
                            ImGui::SameLine();
                            if (moveComp->isMoving) {
                                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Moving");
                                ImGui::Text("  Progress: Node %zu of %zu", moveComp->currentPathIndex,
                                            moveComp->currentPath.size());

                                // actively calculated path
                                if (!moveComp->currentPath.empty()) {
                                    ImGui::Text("  Route:");
                                    for (size_t p = moveComp->currentPathIndex; p < moveComp->currentPath.size(); ++p) {
                                        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "    [%d, %d]",
                                                           moveComp->currentPath[p].q, moveComp->currentPath[p].r);
                                    }
                                }
                            } else {
                                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Idle");
                            }
                            ImGui::Text("  Step Speed: %.2fs", moveComp->timePerStep);
                            ImGui::TreePop();
                        }
                    }
                    if (auto *dirComp = entity.GetComponent<DirectionalTextureComponent>()) {
                        if (ImGui::TreeNodeEx("Directional Texture", ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::Text("  Index: %d", dirComp->index);
                            ImGui::Text("  Textures Bound: %zu", dirComp->textures.size());
                            ImGui::TreePop();
                        }
                    }
                    ImGui::EndTabItem();
                }

                // serialization view
                if (ImGui::BeginTabItem("JSON Preview")) {
                    ImGui::Spacing();
                    ImGui::TextWrapped(
                        "the JSON structure this entity generates when the SceneSerializer requests a save :D");
                    nlohmann::json entityJson;
                    EntityFactory::SerializeEntity(entity, entityJson, *m_Context.resources);
                    std::string jsonStr = entityJson.dump(4);
                    ImGui::InputTextMultiline("##json_preview", jsonStr.data(), jsonStr.size(),
                                              ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        } else {
            ImGui::TextDisabled("\n Click Entity to inspect .-.");
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void Game::Shutdown() const {
}
