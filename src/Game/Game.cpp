#include "Game.h"
#include "Core/Constants.h"
#include "Map/MapSerialization.h"
#include "imgui.h"
#include <filesystem>
#include "Core/Utils.h"
#include "ECS/Components/PlayerInputComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/MovementSystem.h"

// CLEANER BUT STILL  A MESS 4 NOW :)

namespace {
    int g_GridSize = 20;
    int g_SandDensity = 50;
    bool savedOk = false;
    bool hasTestedSave = false;
    bool loadOk = false;
    bool hasTestedLoad = false;
}

bool Game::TestFileWrite(const HexGrid &grid) const {
    std::string path = PathUtils::Join(MAP_PATH, "test", MAP_FILE_EXTENSION);
    size_t expectedBytes = MapIO::CalculateExpectedFileSize(grid.tiles.size());
    MapIO::Serialize(path, grid);
    size_t actualBytes = std::filesystem::file_size(path);
    if (actualBytes == expectedBytes) return true;
    return false;
}

bool Game::TestFileLoad(HexGrid &grid) const {
    std::string path = PathUtils::Join(MAP_PATH, "test", MAP_FILE_EXTENSION);
    return MapIO::Deserialize(path, grid);
}

void Game::Start() {
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
    ImGui::End();

    Scene *activeScene = m_SceneManager.GetCurrentScene();
    if (!activeScene) return;

    ImGui::Begin("obliberry");

    ImGui::Text("Current State:");
    ImGui::SameLine();
    if (m_CurrentState == GameState::Gameplay) ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "GAMEPLAY");
    else if (m_CurrentState == GameState::Paused) ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "PAUSED");

    if (ImGui::Button("Toggle Pause")) {
        m_CurrentState = (m_CurrentState == GameState::Gameplay) ? GameState::Paused : GameState::Gameplay;
    }
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("World Generation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderInt("Hex Radius", &g_GridSize, 2, 100, "%d tiles");
        ImGui::SliderInt("Sand Density", &g_SandDensity, 0, 100, "%d%%");
        if (ImGui::Button("Regenerate Grid", ImVec2(-1, 0))) {
            activeScene->MovePlayerToCenter();
            activeScene->GenerateTiles(g_GridSize, g_SandDensity);
        }

        ImVec4 colorSave = !hasTestedSave
                               ? ImVec4(0.4f, 0.4f, 0.4f, 1.0f)
                               : (savedOk ? ImVec4(0.0f, 0.6f, 0.0f, 1.0f) : ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, colorSave);
        if (ImGui::Button("Run file creation test", ImVec2(-1, 0))) {
            savedOk = TestFileWrite(activeScene->GetGrid());
            hasTestedSave = true;
        }
        ImGui::PopStyleColor(1);

        ImVec4 colorLoad = !hasTestedLoad
                               ? ImVec4(0.4f, 0.4f, 0.4f, 1.0f)
                               : (loadOk ? ImVec4(0.0f, 0.6f, 0.0f, 1.0f) : ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, colorLoad);
        if (ImGui::Button("Run file load test", ImVec2(-1, 0))) {
            loadOk = TestFileLoad(activeScene->GetGrid());
            hasTestedLoad = true;
        }
        ImGui::PopStyleColor(1);
    }

    if (ImGui::CollapsingHeader("Telemetry", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Step Duration", &activeScene->m_PlayerSpeed, 0.05f, 1.0f, "%.2fs per tile");
        ImGui::Separator();

        Entity playerEntity;
        bool foundPlayer = false;
        activeScene->GetRegistry().ForEach<PlayerInputComponent>([&](Entity e, PlayerInputComponent *p) {
            playerEntity = e;
            foundPlayer = true;
        });

        if (foundPlayer) {
            auto *transComp = playerEntity.GetComponent<TransformComponent>();
            auto *moveComp = playerEntity.GetComponent<MovementComponent>();
            if (transComp) {
                HexCoords pCoords = HexMath::PixelToHex({
                    transComp->transform.GetPosition().x, transComp->transform.GetPosition().y
                });
                ImGui::Text("Player Hex Pos:  [Q: %d, R: %d]", pCoords.q, pCoords.r);
            }
            if (moveComp) {
                ImGui::Text("Movement Status: ");
                ImGui::SameLine();
                if (moveComp->isMoving)
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Moving (Node %zu/%zu)",
                                       moveComp->currentPathIndex, moveComp->currentPath.size());
                else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Idle");
            }
        }

        ImGui::Text("Target Hex:      ");
        ImGui::SameLine();
        if (activeScene->m_HasSelection)
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "[Q: %d, R: %d]",
                               activeScene->m_SelectedHex.q, activeScene->m_SelectedHex.r);
        else ImGui::TextDisabled("None");
    }

    if (ImGui::CollapsingHeader("ECS Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {
        Registry &reg = activeScene->GetRegistry();
        const auto &livingEntities = reg.GetLivingEntities();
        ImGui::Text("Active Handle Count: %zu", livingEntities.size());
        ImGui::Separator();

        static int selectedEntityIdx = -1;
        if (selectedEntityIdx >= static_cast<int>(livingEntities.size())) selectedEntityIdx = -1;

        ImGui::BeginChild("EntityListView", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 220.0f), true);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.7f, 1.0f), "Entities");
        ImGui::Separator();

        for (size_t i = 0; i < livingEntities.size(); ++i) {
            EntityID id = livingEntities[i];
            Entity entity(id, &reg);

            std::string label = "ID: " + std::to_string(id);
            if (entity.HasComponent<PlayerInputComponent>()) label += " [Player]";
            else if (entity.HasComponent<MovementComponent>()) label += " [NPC]";

            if (ImGui::Selectable(label.c_str(), selectedEntityIdx == static_cast<int>(i))) {
                selectedEntityIdx = static_cast<int>(i);
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("ComponentInspectorView", ImVec2(0, 220.0f), true);

        if (selectedEntityIdx >= 0 && selectedEntityIdx < static_cast<int>(livingEntities.size())) {
            Entity entity(livingEntities[selectedEntityIdx], &reg);
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Components");
            ImGui::Separator();

            if (auto *transComp = entity.GetComponent<TransformComponent>()) {
                if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    glm::vec3 pos = transComp->transform.GetPosition();
                    if (ImGui::DragFloat3("Position", &pos.x, 0.05f)) transComp->transform.SetPosition(pos);
                    ImGui::TreePop();
                }
            }
            if (auto *moveComp = entity.GetComponent<MovementComponent>()) {
                if (ImGui::TreeNodeEx("Movement", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Text(" State: %s", moveComp->isMoving ? "Moving" : "Idle");
                    ImGui::TreePop();
                }
            }
        } else {
            ImGui::TextDisabled("Select an entity :)");
        }
        ImGui::EndChild();
    }
    ImGui::End();
    // removed some stuff cuz ill readd it later in another ui thingy :)
}

void Game::Shutdown() const {
}
