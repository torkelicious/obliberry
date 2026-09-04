#include "SceneManager.h"
#include "Logger/LoggerService.h"
#include "Core/Constants.h"
#include "Core/EngineContext.h"
#include "Core/Project.h"
#include "Core/Utils/PathUtils.h"
#include "IO/SceneSerialization.h"
#include "IO/VFS/VFS.h"
#include "Scenes/Scene.h"
#include "Rendering/Renderer.h"
#include "Platform/Timeout.h"
#include "ECS/Components/PersistentTagComponent.h"
#include "IO/Loaders/EntityFactory.h"
#include <ObSL/ScriptRuntime.h>
#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <unordered_set>
#include <unordered_map>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "SceneManager"

namespace {
    struct MigratedEntity {
        nlohmann::json data;
        std::optional<size_t> parentBatchIndex;
    };

    void CollectSubtree(const ECS::Entity entity, std::vector<ECS::Entity> &list, std::unordered_set<ECS::EntityID> &visited) {
        const auto id = static_cast<ECS::EntityID>(entity);
        if (visited.contains(id))
            return;

        visited.insert(id);
        list.push_back(entity);

        for (const ECS::EntityID childId : entity.GetChildren()) {
            if (entity.GetRegistry()->IsValid(childId)) {
                CollectSubtree(ECS::Entity(childId, entity.GetRegistry()), list, visited);
            }
        }
    }

    std::vector<MigratedEntity> ExtractPersistentEntities(Scenes::Scene *scene, const Core::EngineContext *context) {
        std::vector<MigratedEntity> batch;
        if (!scene)
            return batch;

        std::vector<ECS::Entity> persistentSubtree;
        std::unordered_set<ECS::EntityID> visited;

        scene->GetRegistry().ForEach<ECS::Components::PersistentTagComponent>([&](const ECS::Entity entity, ECS::Components::PersistentTagComponent *) { CollectSubtree(entity, persistentSubtree, visited); });

        std::unordered_map<ECS::EntityID, size_t> idToBatchIndex;
        for (size_t i = 0; i < persistentSubtree.size(); ++i) {
            idToBatchIndex[static_cast<ECS::EntityID>(persistentSubtree[i])] = i;
        }

        for (auto entity : persistentSubtree) {
            MigratedEntity item;
            IO::EntityFactory::SerializeEntity(entity, item.data, *context->resources);

            if (ECS::Entity parent = entity.GetParent()) {
                if (auto parentId = static_cast<ECS::EntityID>(parent); idToBatchIndex.contains(parentId)) {
                    item.parentBatchIndex = idToBatchIndex[parentId];
                }
            }
            batch.push_back(std::move(item));
        }
        return batch;
    }

    void InjectPersistentEntities(Scenes::Scene *scene, const Core::EngineContext *context, const std::vector<MigratedEntity> &batch) {
        if (!scene || batch.empty())
            return;

        // name based deduplication
        // maybe not the best but ion wanna deal with more complicated shit for now
        std::unordered_set<std::string> batchNames;
        for (const auto &[data, parentBatchIndex] : batch) {
            if (data.contains("name") && !data["name"].is_null()) {
                batchNames.insert(data["name"].get<std::string>());
            }
        }

        if (!batchNames.empty()) {
            std::vector<ECS::EntityID> toDestroy;
            for (const ECS::EntityID entityID : scene->GetRegistry().GetLivingEntities()) {
                if (!scene->GetRegistry().IsValid(entityID))
                    continue;
                if (const std::string &name = scene->GetRegistry().GetEntityName(entityID); !name.empty() && batchNames.contains(name)) {
                    toDestroy.push_back(entityID);
                }
            }
            for (const ECS::EntityID id : toDestroy) {
                scene->GetRegistry().DestroyEntity(id);
            }
        }

        // inject migrated entities
        std::vector<ECS::EntityID> newIds;
        newIds.reserve(batch.size());

        for (const auto &[data, parentBatchIndex] : batch) {
            ECS::EntityID id = scene->GetRegistry().CreateEntity();
            ECS::Entity newEntity(id, &scene->GetRegistry());
            IO::EntityFactory::DeserializeEntity(newEntity, data, *context->resources);
            // PersistentTagComponent is not serialized
            newEntity.AddComponent<ECS::Components::PersistentTagComponent>();
            newIds.push_back(id);
        }

        for (size_t i = 0; i < batch.size(); ++i) {
            if (batch[i].parentBatchIndex.has_value()) {
                const size_t parentIdx = batch[i].parentBatchIndex.value();
                scene->GetRegistry().SetParentDirect(newIds[i], newIds[parentIdx]);
            }
        }
    }
} // namespace

namespace Scenes {
    std::vector<std::string> SceneManager::GetAvailableScenes() {
        std::vector<std::string> scenes;
        if (const auto project = Core::Project::GetActive()) {

            if (const auto scenedir = project->GetAssetsDirectory() / "scenes"; std::filesystem::exists(scenedir)) {
                for (const auto &entry : std::filesystem::directory_iterator(scenedir)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".json") {
                        auto relpath = std::filesystem::relative(entry.path(), project->GetRootDirectory());
                        scenes.push_back(relpath.string());
                    }
                }
            }
        }
        return scenes;
    }

    bool SceneManager::CreateNewScene(const std::string &sceneName) const {
        if (!Core::Project::GetActive()) {
            return false;
        }
        try {
            // sanitize
            std::string safeName = sceneName;
            std::ranges::replace(safeName, ' ', '_');

            const std::string scenepath = Core::PathUtils::Join(Core::SCENE_PATH, safeName, ".json");

            // create
            Scene tempScene(m_Context, SceneProperties{.ScenePath = scenepath, .Name = sceneName, .BackgroundClearColor = {0.1f, 0.1f, 0.1f, 1.0f}});

            return IO::SceneIO::Serialize(scenepath, tempScene);
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to create scene: " + std::string(e.what()));
            return false;
        }
    }

    bool SceneManager::DeleteScene(const std::string &scenePath) {
        if (!Core::Project::GetActive()) {
            return false;
        }
        if (const std::filesystem::path fullPath = Core::Project::GetActive()->GetRootDirectory() / scenePath; std::filesystem::exists(fullPath)) {
            //  unload first
            if (m_CurrentScene && m_CurrentScene->GetScenePath() == scenePath) {
                m_CurrentScene->OnExit();
                m_CurrentScene.reset();
            }
            return std::filesystem::remove(fullPath);
        }
        return false;
    }

    std::string SceneManager::GetCurrentScenePath() const { return m_CurrentScene ? m_CurrentScene->GetScenePath() : std::string{}; }

    void SceneManager::ClearCurrentScene() {
        if (m_CurrentScene) {
            m_CurrentScene->OnExit();
            m_CurrentScene.reset();
        }
    }

    void SceneManager::SwitchScene(const std::string &newScenePath) { LoadSceneByPath(newScenePath); }

    void SceneManager::LoadSceneByPath(const std::string &scenePath) {
        if (!ValidateScenePath(scenePath)) {
            LOG_ERROR(LOG_WHO, "Invalid scene path: " + scenePath);
            return;
        }

        auto newScene = std::make_unique<Scene>(m_Context, SceneProperties{.ScenePath = scenePath});

        LoadScene(std::move(newScene));
    }

    bool SceneManager::SaveCurrentScene() const {
        if (!m_CurrentScene) {
            LOG_ERROR(LOG_WHO, "No current scene to save!");
            return false;
        }

        try {
            const std::string scenePath = m_CurrentScene->GetScenePath();
            if (scenePath.empty()) {
                LOG_ERROR(LOG_WHO, "Current scene has no path!");
                return false;
            }

            if (auto *renderer = m_Context->renderer)
                m_CurrentScene->PostFx() = renderer->GetPostProcessor().Effects();

            const bool success = IO::SceneIO::Serialize(scenePath, *m_CurrentScene);
            if (success) {
                m_CurrentScene->ClearUnsavedChanges();
                LOG_INFO(LOG_WHO, "Successfully saved scene: " + scenePath);
            } else {
                LOG_ERROR(LOG_WHO, "Failed to save scene: " + scenePath);
            }
            return success;
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Exception while saving scene: " + std::string(e.what()));
            return false;
        }
    }

    bool SceneManager::ValidateScenePath(const std::string &scenePath) {
        if (scenePath.empty()) {
            LOG_ERROR(LOG_WHO, "Scene path cannot be empty!");
            return false;
        }

        if (scenePath.find(".json") == std::string::npos) {
            LOG_ERROR(LOG_WHO, "Scene path should have .json extension: " + scenePath);
        }
        return true;
    }

    void SceneManager::ProcessPendingSceneChange(Core::EngineContext &context) {
        if (const std::string scenePath = context.TakePendingScenePath(); !scenePath.empty()) {
            LOG_INFO(LOG_WHO, "Processing pending scene change to: " + scenePath);
            LoadSceneByPath(scenePath);
        }
    }

    void SceneManager::LoadScene(std::unique_ptr<Scene> newScene) {
        std::vector<MigratedEntity> migrationBatch;

        if (m_CurrentScene) {
            migrationBatch = ExtractPersistentEntities(m_CurrentScene.get(), m_Context);
            m_CurrentScene->OnExit();
        }

        if (m_Context->scriptPool) {
            Platform::Time::invalidateGeneration();

            m_Context->scriptPool->shutdown();
            m_Context->scriptPool->init(IO::VFS::GetAssetsDirectory().string() + "/scripts");
        }

        m_CurrentScene = std::move(newScene);

        if (m_CurrentScene) {
            m_CurrentScene->OnEnter();
            InjectPersistentEntities(m_CurrentScene.get(), m_Context, migrationBatch);
        }
    }

    void SceneManager::Update(const float dt) const {
        if (m_CurrentScene) {
            m_CurrentScene->Update(dt);
        }
    }

    void SceneManager::Render() const {
        if (m_CurrentScene) {
            m_CurrentScene->Render();
        }
    }
} // namespace Scenes
#pragma pop_macro("LOG_WHO")
