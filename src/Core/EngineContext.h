#pragma once
#include "Config/GraphicsConfig.h"
#include "Logger/Logger.h"
#include "UI/Rendering/UIRenderer.h"
#include "UI/Rendering/UISystem.h"

#include <filesystem>
#include <mutex>
#include <string>

namespace Config {
    struct ProjectConfig;
}

namespace Platform::Window {
    class Window;
}

namespace Platform::Input {
    class InputManager;
}

namespace Core {
    class Project;
    class ResourceManager;
} // namespace Core

namespace Platform::Threading {
    class ThreadPool;
}

namespace Scenes {
    class SceneManager;
}

namespace Sound {
    class AudioEngine;
}

namespace Rendering {
    class Renderer;
    class Camera;
} // namespace Rendering

namespace ObSL {
    class ScriptRuntime;
}

namespace Scripting {
    class UICommandBuffer;
}

namespace Core {
    struct EngineContext {
        std::filesystem::path ProjectRootPath;

        void SetPendingScenePath(std::string path) {
            std::lock_guard lock(m_PendingSceneMutex);
            m_PendingScenePath = std::move(path);
        }

        [[nodiscard]] std::string TakePendingScenePath() {
            std::lock_guard lock(m_PendingSceneMutex);
            std::string out = std::move(m_PendingScenePath);
            m_PendingScenePath.clear();
            return out;
        }

        [[nodiscard]] bool HasPendingScenePath() {
            std::lock_guard lock(m_PendingSceneMutex);
            return !m_PendingScenePath.empty();
        }

        EngineContext &operator=(const EngineContext &other) {
            if (this != &other) {
                std::lock_guard lockThis(m_PendingSceneMutex);
                ProjectRootPath = other.ProjectRootPath;
                projectConfig = other.projectConfig;
                graphicsConfig = other.graphicsConfig;
                window = other.window;
                input = other.input;
                resources = other.resources;
                sceneManager = other.sceneManager;
                renderer = other.renderer;
                camera = other.camera;
                uiRenderer = other.uiRenderer;
                uiSystem = other.uiSystem;
                uiCmdBuf = other.uiCmdBuf;
                scriptPool = other.scriptPool;
                threadPool = other.threadPool;
                audioEngine = other.audioEngine;
                deltaTime = other.deltaTime;
                timeScale = other.timeScale;
                frameCount = other.frameCount;
                activeProject = other.activeProject;
                isEditorMode = other.isEditorMode;
                logger = other.logger;
            }
            return *this;
        }

        Config::ProjectConfig *projectConfig = nullptr;
        Config::GraphicsConfig *graphicsConfig = nullptr;
        Platform::Window::Window *window = nullptr;
        Platform::Input::InputManager *input = nullptr;
        ResourceManager *resources = nullptr;
        Scenes::SceneManager *sceneManager = nullptr;
        Rendering::Renderer *renderer = nullptr;
        Rendering::Camera *camera = nullptr;
        UI::UIRenderer *uiRenderer = nullptr;
        UI::UISystem *uiSystem = nullptr;
        Scripting::UICommandBuffer *uiCmdBuf = nullptr;
        ObSL::ScriptRuntime *scriptPool = nullptr;
        Platform::Threading::ThreadPool *threadPool = nullptr;
        Sound::AudioEngine *audioEngine = nullptr;
        float deltaTime = 0.0f;
        float timeScale = 1.0f;
        uint64_t frameCount = 0;
        std::shared_ptr<Project> activeProject = nullptr;
        bool isEditorMode = false;
        // Logging
        Logging::ILogger *logger = nullptr;

    private:
        std::mutex m_PendingSceneMutex;
        std::string m_PendingScenePath;
    };
} // namespace Core
