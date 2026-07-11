#include "Core/LoggerService.h"

#include "GameLayer.h"


constexpr auto LOG_WHO = "GameUI";
#include "imgui.h"
#include "IO/MapSerialization.h"
#include "ECS/Systems/MapRuntimeSystem.h"
#include "IO/SceneSerialization.h"
#include "glm/glm.hpp"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/BillboardTagComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/PointLightComponent.h"

namespace {
    [[nodiscard]] ImVec4 GetThresholdColor(const float frameMs) {
        if (frameMs >= 33.0f)
            return {1.0f, 0.3f, 0.3f, 1.0f}; // critical (sub-30 FPS)
        if (frameMs >= 20.0f)
            return {1.0f, 0.8f, 0.2f, 1.0f}; // warning (sub-50 FPS)
        return {0.4f, 1.0f, 0.4f, 1.0f};     // Good
    }

    bool showPerformanceOverlay = true;
    bool showGameState = true;
    bool showEntityInspector = true;
    bool showSceneSwitcher = true;
} // namespace

void Game::GameLayer::DrawInterface() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("obliberry")) {
            ImGui::MenuItem("Performance Overlay", nullptr, &showPerformanceOverlay);
            ImGui::MenuItem("GameLayer State", nullptr, &showGameState);
            ImGui::MenuItem("Entity Inspector", nullptr, &showEntityInspector);
            ImGui::MenuItem("Scene / Map IO", nullptr, &showSceneSwitcher);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (showPerformanceOverlay) {
        constexpr float PADDING = 10.0f;
        const ImGuiViewport *viewport = ImGui::GetMainViewport();

        const float menuBarHeight = ImGui::GetFrameHeight();

        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - PADDING, viewport->WorkPos.y + PADDING + menuBarHeight), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.40f);
        constexpr ImGuiWindowFlags overlayFlags =
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

        if (ImGui::Begin("Performance", &showPerformanceOverlay, overlayFlags)) {
            ImGui::TextColored({0.0f, 1.0f, 0.7f, 1.0f}, "perf");
            ImGui::Separator();
            const float frameMs = m_Context->deltaTime * 1000.0f;
            ImGui::Text("FPS:    %.1f", ImGui::GetIO().Framerate);
            ImGui::TextColored(GetThresholdColor(frameMs), "Delta: %.2f ms", frameMs);
        }
        ImGui::End();
    }

    if (showGameState) {
        if (ImGui::Begin("GameLayer State", &showGameState)) {
            ImGui::Checkbox("is running", &m_GameIsRunning);

            ImGui::Separator();
            if (m_SceneManager.GetCurrentScene()) {
                ImGui::Text("Active Scene: Loaded");
            } else {
                ImGui::Text("Active Scene: None");
            }
        }
        ImGui::End();
    }

    if (showEntityInspector && m_SceneManager.GetCurrentScene()) {
        if (ImGui::Begin("Entity Inspector", &showEntityInspector)) {
            auto &registry = m_SceneManager.GetCurrentScene()->GetRegistry();
            const auto &livingEntities = registry.GetLivingEntities();
            static auto selectedEntity = static_cast<ECS::EntityID>(-1);
            if (selectedEntity != static_cast<ECS::EntityID>(-1) && !registry.IsValid(selectedEntity)) {
                selectedEntity = static_cast<ECS::EntityID>(-1);
            }

            ImGui::BeginChild("Entity List", ImVec2(150, 0), true);
            for (const ECS::EntityID id : livingEntities) {
                ECS::Entity entity(id, &registry);
                std::string label = entity.GetName();
                if (label.empty()) {
                    label = "Entity " + std::to_string(id);
                }
                ImGui::PushID(id);
                if (ImGui::Selectable(label.c_str(), selectedEntity == id)) {
                    selectedEntity = id;
                }
                ImGui::PopID();
            }
            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::BeginChild("Component View", ImVec2(0, 0), true);
            if (selectedEntity != static_cast<ECS::EntityID>(-1)) {
                const ECS::Entity entity(selectedEntity, &registry);
                std::string headerName = entity.GetName();
                if (headerName.empty()) {
                    headerName = "Entity " + std::to_string(selectedEntity);
                }
                ImGui::Text("%s Components", headerName.c_str());
                ImGui::Separator();

                // todo: make special thing for component ui's
                // to be used in editor or something later on??
                // this is a lil messy..
                // TransformComponent
                if (entity.HasComponent<ECS::Components::TransformComponent>()) {
                    if (ImGui::CollapsingHeader("Transform Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                        auto *tc = entity.GetComponent<ECS::Components::TransformComponent>();
                        glm::vec3 pos = tc->transform.GetPosition();
                        glm::vec3 rot = tc->transform.GetRotation();
                        glm::vec3 scale = tc->transform.GetScale();

                        if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
                            tc->transform.SetPosition(pos);
                        }
                        if (ImGui::DragFloat3("Rotation", &rot.x, 0.05f)) {
                            tc->transform.SetRotation(rot);
                        }
                        if (ImGui::DragFloat3("Scale", &scale.x, 0.1f)) {
                            tc->transform.SetScale(scale);
                        }
                    }
                }

                // MapComponent
                if (entity.HasComponent<ECS::Components::MapComponent>()) {
                    if (ImGui::CollapsingHeader("Map Component")) {
                        auto *mc = entity.GetComponent<ECS::Components::MapComponent>();
                        ImGui::Text("Map File: %s", mc->mapFilePath.c_str());
                        ImGui::Checkbox("Needs Mesh Update", &mc->needsMeshUpdate);
                    }
                }

                // MapStateComponent
                if (entity.HasComponent<ECS::Components::MapStateComponent>()) {
                    if (ImGui::CollapsingHeader("Map State Component")) {
                        auto *msc = entity.GetComponent<ECS::Components::MapStateComponent>();
                        ImGui::Checkbox("Has Selection", &msc->hasSelection);
                        ImGui::Checkbox("Has Path To", &msc->hasPathTo);
                    }
                }

                // PointLightComponent
                if (entity.HasComponent<ECS::Components::PointLightComponent>()) {
                    if (ImGui::CollapsingHeader("Point Light Component")) {
                        auto *plc = entity.GetComponent<ECS::Components::PointLightComponent>();
                        ImGui::ColorEdit3("Color", &plc->color.x);
                        ImGui::DragFloat("Radius", &plc->radius, 0.5f, 0.0f, 1000.0f);
                        ImGui::DragFloat("Intensity", &plc->intensity, 0.05f, 0.0f, 10.0f);
                    }
                }

                // DirectionalTextureComponent
                if (entity.HasComponent<ECS::Components::DirectionalTextureComponent>()) {
                    if (ImGui::CollapsingHeader("Directional Texture Component")) {
                        auto *dtc = entity.GetComponent<ECS::Components::DirectionalTextureComponent>();
                        ImGui::SliderInt("Sprite Index", &dtc->index, 0, 5);
                    }
                }

                // MovementComponent
                if (entity.HasComponent<ECS::Components::MovementComponent>()) {
                    if (ImGui::CollapsingHeader("Movement Component")) {
                        auto *mov = entity.GetComponent<ECS::Components::MovementComponent>();
                        ImGui::Checkbox("Is Moving", &mov->isMoving);
                        ImGui::DragFloat("Time Per Step", &mov->timePerStep, 0.01f, 0.01f, 5.0f);
                        ImGui::Text("Path Length: %zu", mov->currentPath.size());
                        ImGui::Text("Current Path Index: %zu", mov->currentPathIndex);

                        // progress bar
                        const float progress = mov->timePerStep > 0.0f ? mov->stepTimer / mov->timePerStep : 0.0f;
                        ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), "Step Progress");
                    }
                }

                // MeshComponent
                if (entity.HasComponent<ECS::Components::MeshComponent>()) {
                    if (ImGui::CollapsingHeader("Mesh Component")) {
                        const auto *mc = entity.GetComponent<ECS::Components::MeshComponent>();
                        ImGui::Text("Mesh Loaded: %s", mc->mesh ? "Yes" : "No");
                    }
                }

                // MaterialComponent
                if (entity.HasComponent<ECS::Components::MaterialComponent>()) {
                    if (ImGui::CollapsingHeader("Material Component")) {
                        const auto *mat = entity.GetComponent<ECS::Components::MaterialComponent>();
                        ImGui::Text("Material Loaded: %s", mat->material ? "Yes" : "No");
                    }
                }

                // BillboardTagComponent
                if (entity.HasComponent<ECS::Components::BillboardTagComponent>()) {
                    if (ImGui::CollapsingHeader("Billboard Component")) {
                        ImGui::TextWrapped("(Tag Component)");
                    }
                }
            } else {
                ImGui::Text("Select an entity :)");
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }

    if (showSceneSwitcher) {
        if (ImGui::Begin("Scene / Map IO", &showSceneSwitcher)) {
            static char nameBuf[256] = "";
            ImGui::InputText("Name", nameBuf, sizeof(nameBuf));

            if (ImGui::Button("Load Scene")) {
                m_PendingSceneLoad = Scenes::SceneProperties{Core::PathUtils::Join(Core::SCENE_PATH, nameBuf, Core::SCENE_FILE_EXTENSION)};
            }
            ImGui::SameLine();
            if (ImGui::Button("Save Scene") && m_SceneManager.GetCurrentScene()) {
                const std::string savePath = Core::PathUtils::Join(Core::SCENE_PATH, nameBuf, Core::SCENE_FILE_EXTENSION);
                IO::SceneIO::Serialize(savePath, *m_SceneManager.GetCurrentScene());
            }

            if (ImGui::Button("Load Map") && m_SceneManager.GetCurrentScene()) {
                auto &scene = *m_SceneManager.GetCurrentScene();
                auto &registry = scene.GetRegistry();
                registry.ForEach<ECS::Components::MapComponent>([&](ECS::Entity, ECS::Components::MapComponent *mapComp) {
                    const std::string newPath = Core::PathUtils::Join(Core::MAP_PATH, nameBuf, Core::MAP_FILE_EXTENSION);
                    mapComp->mapFilePath = newPath;
                    if (IO::MapIO::Deserialize(newPath, mapComp->grid)) {
                        ECS::Systems::MapRuntimeSystem::OnMapChanged(registry, scene.GetContext());
                    }
                });
            }

            ImGui::SameLine();
            if (ImGui::Button("Save Map") && m_SceneManager.GetCurrentScene()) {
                auto &scene = *m_SceneManager.GetCurrentScene();
                auto &registry = scene.GetRegistry();
                registry.ForEach<ECS::Components::MapComponent>([&](ECS::Entity, const ECS::Components::MapComponent *mapComp) {
                    const std::string newPath = Core::PathUtils::Join(Core::MAP_PATH, nameBuf, Core::MAP_FILE_EXTENSION);
                    if ([[maybe_unused]] const bool ok = IO::MapIO::Serialize(newPath, mapComp->grid)) {
                        LOG_INFO(LOG_WHO, "Saved!");
                    }
                });
            }
        }
        ImGui::End();
    }
}
