#include "Game.h"
#include "Core/Constants.h"
#include "ECS/Components/BillboardComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/PlayerInputComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/MapRuntimeSystem.h"
#include "IO/EntityFactory.h"
#include "IO/SceneSerialization.h"
#include "Map/Hex.h"
#include "Map/HexMath.h"
#include "imgui.h"
#include <algorithm>
#include <random>
#include <string_view>

namespace {
    // persistent UI states
    int g_GridSize = 20;
    int g_SandDensity = 50;
    bool g_SaveOk = false;
    bool g_HasTestedSave = false;
    bool g_LoadOk = false;
    bool g_HasTestedLoad = false;

    [[nodiscard]] std::string ExtractFilename(std::string_view fullPath,
                                              std::string_view prefix,
                                              std::string_view extension) {
        if (fullPath.starts_with(prefix)) fullPath.remove_prefix(prefix.size());
        if (fullPath.ends_with(extension)) fullPath.remove_suffix(extension.size());
        return std::string(fullPath);
    }

    [[nodiscard]] std::string FilenameInputLine(std::string_view prefix,
                                                std::string_view extension,
                                                char *buf, size_t bufSize,
                                                const char *widgetId) {
        const float avail = ImGui::GetContentRegionAvail().x;
        const float preW = ImGui::CalcTextSize(prefix.data(),
                                               prefix.data() + prefix.size()).x;
        const float extW = ImGui::CalcTextSize(extension.data(),
                                               extension.data() + extension.size()).x;
        const float inpW = std::max(avail - preW - extW - 4.0f, 40.0f);

        ImGui::TextDisabled("%.*s", (int) prefix.size(), prefix.data());
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::SetNextItemWidth(inpW);
        ImGui::InputText(widgetId, buf, bufSize);
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::TextDisabled("%.*s", (int) extension.size(), extension.data());

        std::string result;
        result.reserve(prefix.size() + std::string_view(buf).size() + extension.size());
        result.append(prefix);
        result.append(std::string_view(buf));
        result.append(extension);
        return result;
    }

    [[nodiscard]] ImVec4 ThresholdColour(float value, float warn, float crit) {
        if (value >= crit) return {1.0f, 0.3f, 0.3f, 1.0f};
        if (value >= warn) return {1.0f, 0.8f, 0.2f, 1.0f};
        return {0.4f, 1.0f, 0.4f, 1.0f};
    }
}

void Game::DrawInterface() {
    constexpr float PADDING = 10.0f;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(
        ImVec2((viewport->WorkPos.x + viewport->WorkSize.x) - PADDING,
               viewport->WorkPos.y + PADDING),
        ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.40f);
    ImGui::Begin("Performance", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.7f, 1.0f), "perf");
    ImGui::Separator();
    const float frameMs = m_Context.deltaTime * 1000.0f;
    ImGui::Text("FPS:    %.1f", ImGui::GetIO().Framerate);
    ImGui::TextColored(ThresholdColour(frameMs, 20.0f, 33.0f), "Delta: %.2f ms", frameMs);
    ImGui::End();

    Scene *activeScene = m_SceneManager.GetCurrentScene();
    if (!activeScene) return;

    Registry &reg = activeScene->GetRegistry();

    MapComponent *mapComp = nullptr;
    MapStateComponent *stateComp = nullptr;
    reg.ForEach<MapComponent, MapStateComponent>(
        [&](Entity, MapComponent *m, MapStateComponent *s) {
            mapComp = m;
            stateComp = s;
        });

    ImGui::Begin("obliberry");

    ImGui::Text("State:");
    ImGui::SameLine();
    if (m_CurrentState == GameState::Gameplay)
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Gameplay");
    else if (m_CurrentState == GameState::Paused)
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Paused");

    if (ImGui::Button(m_CurrentState == GameState::Gameplay ? "Pause" : "Resume",
                      ImVec2(-1, 0))) {
        m_CurrentState = (m_CurrentState == GameState::Gameplay)
                             ? GameState::Paused
                             : GameState::Gameplay;
    }
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
        static char s_SceneFilename[128]{};
        static std::string s_LastScenePath;
        const std::string &currentPath = activeScene->GetScenePath();
        if (currentPath != s_LastScenePath) {
            const std::string fn =
                    ExtractFilename(currentPath, SCENE_PATH, SCENE_FILE_EXTENSION);
            const size_t copyLen = std::min(fn.size(), sizeof(s_SceneFilename) - 1);
            fn.copy(s_SceneFilename, copyLen);
            s_SceneFilename[copyLen] = '\0';
            s_LastScenePath = currentPath;
        }

        const std::string scenePath = FilenameInputLine(
            SCENE_PATH, SCENE_FILE_EXTENSION,
            s_SceneFilename, sizeof(s_SceneFilename), "##scenefilename");
        ImGui::Spacing();

        static bool s_SceneSaved = false;
        static bool s_SceneSaveTried = false;
        static std::string s_LastInputScenePath;

        if (scenePath != s_LastInputScenePath) {
            s_SceneSaveTried = false;
            s_SceneSaved = false;
            s_LastInputScenePath = scenePath;
        }

        const ImVec4 saveCol = !s_SceneSaveTried
                                   ? ImVec4(0.4f, 0.4f, 0.4f, 1.0f)
                                   : (s_SceneSaved
                                          ? ImVec4(0.0f, 0.6f, 0.0f, 1.0f)
                                          : ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, saveCol);
        if (ImGui::Button("Save Scene",
                          ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 4.0f, 0))) {
            s_SceneSaved = SceneIO::Serialize(scenePath, *activeScene);
            s_SceneSaveTried = true;
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Button("Reload Scene", ImVec2(-1, 0)))
            m_PendingSceneLoad = scenePath;
    }
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("World & Data", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (mapComp) {
            static int s_Grass = 0, s_Sand = 0, s_Walkable = 0;
            static bool s_StatsValid = false;
            static const MapComponent *s_LastMapComp = nullptr;

            if (!s_StatsValid || mapComp->needsMeshUpdate || s_LastMapComp != mapComp) {
                s_Grass = s_Sand = s_Walkable = 0;
                for (const auto &[c, t]: mapComp->grid.tiles) {
                    if (t.type == TileType::Grass) ++s_Grass;
                    else ++s_Sand;
                    if (t.walkable) ++s_Walkable;
                }
                s_StatsValid = true;
                s_LastMapComp = mapComp;
            }

            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
                               "Grid: %zu tiles", mapComp->grid.tiles.size());
            ImGui::Indent(8.0f);
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Grass:    %d", s_Grass);
            ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "Sand:     %d", s_Sand);
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.7f, 1.0f), "Walkable: %d / %zu",
                               s_Walkable, mapComp->grid.tiles.size());
            ImGui::Unindent(8.0f);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                               "Map: %s", mapComp->mapFilePath.c_str());
            ImGui::Separator();
        }

        ImGui::SliderInt("Hex Radius", &g_GridSize, 2, 100, "%d tiles");
        ImGui::SliderInt("Sand Density", &g_SandDensity, 0, 100, "%d%%");

        if (ImGui::Button("Regenerate Grid", ImVec2(-1, 0))) {
            if (mapComp) {
                mapComp->grid.Clear();
                std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<int> dist(0, 99);
                for (int q = -g_GridSize; q < g_GridSize; ++q)
                    for (int r = -g_GridSize; r < g_GridSize; ++r)
                        mapComp->grid.EmplaceTile(
                            {q, r},
                            dist(rng) < g_SandDensity ? TileType::Sand : TileType::Grass);
                MapRuntimeSystem::OnMapChanged(reg, *mapComp, stateComp, m_Context);
            }
        }

        ImGui::Spacing();
        ImGui::Text("Map I/O:");

        static char s_MapFilename[128]{};
        static std::string s_LastMapPath;
        if (mapComp && mapComp->mapFilePath != s_LastMapPath) {
            const std::string fn =
                    ExtractFilename(mapComp->mapFilePath, MAP_PATH, MAP_FILE_EXTENSION);
            const size_t copyLen = std::min(fn.size(), sizeof(s_MapFilename) - 1);
            fn.copy(s_MapFilename, copyLen);
            s_MapFilename[copyLen] = '\0';
            s_LastMapPath = mapComp->mapFilePath;
        }

        const std::string mapPath = FilenameInputLine(
            MAP_PATH, MAP_FILE_EXTENSION,
            s_MapFilename, sizeof(s_MapFilename), "##mapfilename");
        ImGui::Spacing();

        static std::string s_LastInputMapPath;
        if (mapPath != s_LastInputMapPath) {
            g_HasTestedSave = false;
            g_HasTestedLoad = false;
            g_SaveOk = false;
            g_LoadOk = false;
            s_LastInputMapPath = mapPath;
        }

        const ImVec4 colorSave = !g_HasTestedSave
                                     ? ImVec4(0.4f, 0.4f, 0.4f, 1.0f)
                                     : (g_SaveOk ? ImVec4(0.0f, 0.6f, 0.0f, 1.0f) : ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, colorSave);
        if (ImGui::Button("Save Map",
                          ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 4.0f, 0))) {
            if (mapComp) {
                g_SaveOk = TestFileWrite(mapComp->grid, mapPath);
                g_HasTestedSave = true;
                if (g_SaveOk) {
                    mapComp->mapFilePath = mapPath;
                    s_LastMapPath = mapPath;
                }
            }
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();

        const ImVec4 colorLoad = !g_HasTestedLoad
                                     ? ImVec4(0.4f, 0.4f, 0.4f, 1.0f)
                                     : (g_LoadOk ? ImVec4(0.0f, 0.6f, 0.0f, 1.0f) : ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, colorLoad);
        if (ImGui::Button("Load Map", ImVec2(-1, 0))) {
            if (mapComp) {
                g_LoadOk = TestFileLoad(mapComp->grid, mapPath);
                g_HasTestedLoad = true;
                if (g_LoadOk) {
                    mapComp->mapFilePath = mapPath;
                    s_LastMapPath = mapPath;
                    MapRuntimeSystem::OnMapChanged(reg, *mapComp, stateComp, m_Context);
                }
            }
        }
        ImGui::PopStyleColor();
    }
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Camera & Input", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (m_Context.camera) {
            ImGui::Text("Pos:  [%.2f, %.2f]",
                        m_Context.camera->Position.x, m_Context.camera->Position.y);
            ImGui::Text("Zoom: %.2fx", m_Context.camera->Zoom);
            ImGui::Separator();
        }

        ImGui::Text("Mouse hover:");
        if (stateComp && stateComp->hasSelection) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "  Hex [Q:%d, R:%d]",
                               stateComp->selectedHex.q, stateComp->selectedHex.r);
            if (mapComp && mapComp->grid.HasTile(stateComp->selectedHex)) {
                const TileType type = mapComp->grid.Get(stateComp->selectedHex)->type;
                ImGui::Text("  Type: %s", type == TileType::Grass ? "Grass" : "Sand");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "  (void)");
            }
        } else {
            ImGui::TextDisabled("  (void)");
        }

        if (stateComp && stateComp->hasPathTo) {
            ImGui::Text("Path destination:");
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "  Hex [Q:%d, R:%d]",
                               stateComp->pathTo.q, stateComp->pathTo.r);
        }
    }
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("ECS", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto &livingEntities = reg.GetLivingEntities();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f),
                           "Active Entities: %zu", livingEntities.size());
        ImGui::Separator();

        static EntityID s_SelectedEntityID = static_cast<EntityID>(-1);
        static EntityID s_LastSelectedEntityID = static_cast<EntityID>(-1);
        static std::string s_CachedJson;

        ImGui::BeginChild("EntityList",
                          ImVec2(ImGui::GetContentRegionAvail().x * 0.35f, 380.0f), true);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.7f, 1.0f), "Handles");
        ImGui::Separator();

        for (size_t i = 0; i < livingEntities.size(); ++i) {
            const EntityID id = livingEntities[i];
            Entity entity(id, &reg);

            std::string label = "ID: " + std::to_string(id);
            if (entity.HasComponent<PlayerInputComponent>()) label += " [Player]";
            else if (entity.HasComponent<MovementComponent>()) label += " [NPC]";
            else if (entity.HasComponent<MapComponent>()) label += " [Map]";

            if (ImGui::Selectable(label.c_str(), s_SelectedEntityID == id))
                s_SelectedEntityID = id;
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("ComponentInspector", ImVec2(0, 380.0f), true);

        bool isSelectedAlive = std::find(livingEntities.begin(), livingEntities.end(), s_SelectedEntityID) !=
                               livingEntities.end();

        if (s_SelectedEntityID != static_cast<EntityID>(-1) && isSelectedAlive) {
            Entity entity(s_SelectedEntityID, &reg);

            if (ImGui::BeginTabBar("EntityTabs")) {
                if (ImGui::BeginTabItem("Components")) {
                    ImGui::Spacing();

                    if (auto *tc = entity.GetComponent<TransformComponent>()) {
                        if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                            const glm::vec3 pos = tc->transform.GetPosition();
                            ImGui::Text("  World: [%.2f, %.2f, %.2f]", pos.x, pos.y, pos.z);
                            const HexCoords hc = HexMath::PixelToHex({pos.x, pos.y});
                            ImGui::Text("  Hex:   [Q:%d, R:%d]", hc.q, hc.r);
                            ImGui::TreePop();
                        }
                    }

                    if (auto *mc = entity.GetComponent<MovementComponent>()) {
                        if (ImGui::TreeNodeEx("Movement", ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::Text("  Status: ");
                            ImGui::SameLine();
                            if (mc->isMoving) {
                                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Moving");
                                ImGui::Text("  Node %zu / %zu",
                                            mc->currentPathIndex, mc->currentPath.size());
                                for (size_t p = mc->currentPathIndex;
                                     p < mc->currentPath.size(); ++p) {
                                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                                                       "    [%d, %d]",
                                                       mc->currentPath[p].q,
                                                       mc->currentPath[p].r);
                                }
                            } else {
                                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Idle");
                            }
                            ImGui::Text("  Step: %.2fs", mc->timePerStep);
                            ImGui::TreePop();
                        }
                    }

                    if (auto *dc = entity.GetComponent<DirectionalTextureComponent>()) {
                        if (ImGui::TreeNodeEx("Directional Texture",
                                              ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::Text("  Index:    %d / 5", dc->index);
                            ImGui::Text("  Textures: %zu", dc->textures.size());
                            ImGui::TreePop();
                        }
                    }

                    if (entity.HasComponent<BillboardComponent>()) {
                        if (ImGui::TreeNodeEx("Billboard", ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::TextColored(ImVec4(0.5f, 0.9f, 1.0f, 1.0f),
                                               "  Camera-facing: active");
                            ImGui::TreePop();
                        }
                    }

                    if (auto *ms = entity.GetComponent<MapStateComponent>()) {
                        if (ImGui::TreeNodeEx("Map State", ImGuiTreeNodeFlags_DefaultOpen)) {
                            if (ms->hasSelection)
                                ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f),
                                                   "  Hover: [Q:%d, R:%d]",
                                                   ms->selectedHex.q, ms->selectedHex.r);
                            else
                                ImGui::TextDisabled("  No hover");
                            if (ms->hasPathTo)
                                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                                                   "  Path dest: [Q:%d, R:%d]",
                                                   ms->pathTo.q, ms->pathTo.r);
                            else
                                ImGui::TextDisabled("  No path destination");
                            ImGui::TreePop();
                        }
                    }

                    if (auto *map = entity.GetComponent<MapComponent>()) {
                        if (ImGui::TreeNodeEx("Map", ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::Text("  Tiles:      %zu", map->grid.tiles.size());
                            ImGui::Text("  Walkable:   %zu", map->grid.walkableTiles.size());
                            ImGui::Text("  Mesh dirty: %s",
                                        map->needsMeshUpdate ? "yes" : "no");
                            ImGui::TreePop();
                        }
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("JSON Preview")) {
                    ImGui::Spacing();
                    ImGui::TextWrapped("Serialized JSON this entity produces on save.");

                    if (s_SelectedEntityID != s_LastSelectedEntityID || ImGui::Button("Refresh")) {
                        nlohmann::json j;
                        EntityFactory::SerializeEntity(entity, j, *m_Context.resources);
                        s_CachedJson = j.dump(4);
                        s_LastSelectedEntityID = s_SelectedEntityID;
                    }

                    ImGui::InputTextMultiline(
                        "##json", const_cast<char *>(s_CachedJson.c_str()), s_CachedJson.capacity() + 1,
                        ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        } else {
            ImGui::TextDisabled("\n  Select an entity to inspect.");
            if (!isSelectedAlive) s_SelectedEntityID = static_cast<EntityID>(-1);
        }

        ImGui::EndChild();
    }

    ImGui::End();
}
