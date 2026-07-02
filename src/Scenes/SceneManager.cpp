#include "SceneManager.h"
#include "Core/Constants.h"
#include "Core/EngineContext.h"
#include "Core/Project.h"
#include "Core/Utils.h"
#include "IO/SceneSerialization.h"
#include "Scenes/Scene.h"
#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace Scenes {
    std::vector<std::string> SceneManager::GetAvailableScenes() const {
        std::vector<std::string> scenes;
        if (const auto project = Core::Project::GetActive()) {
            const auto scenedir = project->GetAssetsDirectory() / "scenes";

            if (std::filesystem::exists(scenedir)) {
                for (const auto &entry: std::filesystem::directory_iterator(scenedir)) {
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
            Scene tempScene(m_Context, SceneProperties{
                                .ScenePath = scenepath,
                                .Name = sceneName,
                                .BackgroundClearColor = {0.1f, 0.1f, 0.1f, 1.0f}
                            });


            return IO::SceneIO::Serialize(scenepath, tempScene);
        } catch (const std::exception &e) {
            std::cerr << "Failed to create scene: " << e.what() << std::endl;
            return false;
        }
    }

    bool SceneManager::DeleteScene(const std::string &scenePath) {
        if (!Core::Project::GetActive()) {
            return false;
        }
        const std::filesystem::path fullPath = Core::Project::GetActive()->GetRootDirectory() / scenePath;
        if (std::filesystem::exists(fullPath)) {
            //  unload first
            if (m_CurrentScene &&m_CurrentScene



            ->
            GetScenePath() == scenePath
            )
            {
                m_CurrentScene->OnExit();
                m_CurrentScene.reset();
            }
            return std::filesystem::remove(fullPath);
        }
        return false;
    }

    std::string SceneManager::GetCurrentScenePath() const {
        return m_CurrentScene ? m_CurrentScene->GetScenePath() : std::string{};
    }

    void SceneManager::ClearCurrentScene() {
        if (m_CurrentScene) {
            m_CurrentScene->OnExit();
            m_CurrentScene.reset();
        }
    }

    void SceneManager::SwitchScene(const std::string &newScenePath) {
        // save current scene if it exists and has changes
        // TODO: add dialog in ui asking if to save type shit
        if (m_CurrentScene) {
            if (m_CurrentScene->HasUnsavedChanges()) {
                IO::SceneIO::Serialize(m_CurrentScene->GetScenePath(), *m_CurrentScene);
            }
        }

        LoadSceneByPath(newScenePath);
    }

    void SceneManager::LoadSceneByPath(const std::string &scenePath) {
        if (!ValidateScenePath(scenePath)) {
            std::cerr << "[SceneManager] Invalid scene path: " << scenePath << std::endl;
            return;
        }

        auto newScene = std::make_unique<Scene>(
            m_Context,
            SceneProperties{.ScenePath = scenePath}
        );

        LoadScene(std::move(newScene));
    }

    bool SceneManager::SaveCurrentScene() const {
        if (!m_CurrentScene) {
            std::cerr << "[SceneManager] No current scene to save!" << std::endl;
            return false;
        }

        try {
            const std::string scenePath = m_CurrentScene->GetScenePath();
            if (scenePath.empty()) {
                std::cerr << "[SceneManager] Current scene has no path!" << std::endl;
                return false;
            }

            const bool success = IO::SceneIO::Serialize(scenePath, *m_CurrentScene);
            if (success) {
                m_CurrentScene->ClearUnsavedChanges();
                std::cout << "[SceneManager] Successfully saved scene: " << scenePath << std::endl;
            } else {
                std::cerr << "[SceneManager] Failed to save scene: " << scenePath << std::endl;
            }
            return success;
        } catch (const std::exception &e) {
            std::cerr << "[SceneManager] Exception while saving scene: " << e.what() << std::endl;
            return false;
        }
    }

    bool SceneManager::ValidateScenePath(const std::string &scenePath) const {
        if (scenePath.empty()) {
            std::cerr << "[SceneManager] Scene path cannot be empty!" << std::endl;
            return false;
        }

        if (scenePath.find(".json") == std::string::npos) {
            std::cerr << "[SceneManager] Scene path should have .json extension: " << scenePath << std::endl;
        }
        return true;
    }

    void SceneManager::ProcessPendingSceneChange(Core::EngineContext &context) {
        if (!context.pendingScenePath.empty()) {
            const std::string scenePath = std::move(context.pendingScenePath);
            context.pendingScenePath.clear();

            std::cout << "[SceneManager] Processing pending scene change to: " << scenePath << std::endl;
            LoadSceneByPath(scenePath);
        }
    }

    void SceneManager::LoadScene(std::unique_ptr<Scene> newScene) {
        if (m_CurrentScene) {
            m_CurrentScene->OnExit();
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
