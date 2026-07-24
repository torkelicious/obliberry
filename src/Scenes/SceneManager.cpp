#include "SceneManager.h"
#include "Logger/LoggerService.h"
#include "Core/Constants.h"
#include "Core/EngineContext.h"
#include "Core/Project.h"
#include "Core/Utils/PathUtils.h"
#include "IO/SceneSerialization.h"
#include "Scenes/Scene.h"
#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "SceneManager"

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
            if (m_CurrentScene && m_CurrentScene


                                                  ->GetScenePath() == scenePath) {
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

    bool SceneManager::ValidateScenePath(const std::string &scenePath) const {
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
        if (!context.pendingScenePath.empty()) {
            const std::string scenePath = std::move(context.pendingScenePath);
            context.pendingScenePath.clear();

            LOG_INFO(LOG_WHO, "Processing pending scene change to: " + scenePath);
            LoadSceneByPath(scenePath);
        }
    }

    void SceneManager::LoadScene(std::unique_ptr<Scene> newScene) {
        if (m_CurrentScene) {
            m_CurrentScene->OnExit();
        }

        if (m_Context->scriptPool) {
            m_Context->scriptPool->shutdown();
            m_Context->scriptPool->init(IO::VFS::GetAssetsDirectory() / "scripts");
        }

        m_CurrentScene = std::move(newScene);

        if (m_CurrentScene) {
            m_CurrentScene->OnEnter();
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
