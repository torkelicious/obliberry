#pragma once

#include "Core/Logger.h"

#include <filesystem>
#include <string>

namespace Core {
    class Project;
    struct ProjectConfig;
    class Window;
    class InputManager;
    class ResourceManager;
    class ThreadPool;
} // namespace Core

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

namespace Core {
    struct EngineContext {
        std::filesystem::path ProjectRootPath;
        std::string pendingScenePath; // for deferred scene loading (GameLayer, Scripting)
        ProjectConfig *projectConfig = nullptr;
        Window *window = nullptr;
        InputManager *input = nullptr;
        ResourceManager *resources = nullptr;
        Scenes::SceneManager *sceneManager = nullptr;
        Rendering::Renderer *renderer = nullptr;
        Rendering::Camera *camera = nullptr;
        ObSL::ScriptRuntime *scriptPool = nullptr;
        ThreadPool *threadPool = nullptr;
        Sound::AudioEngine *audioEngine = nullptr;
        float deltaTime = 0.0f;
        float timeScale = 1.0f;
        uint64_t frameCount = 0;
        std::shared_ptr<Project> activeProject = nullptr;
        // Logging
        Logging::ILogger *logger = nullptr;
    };
} // namespace Core
