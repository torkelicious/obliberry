#pragma once


#include <filesystem>
#include <string>

namespace Core {
    class Project;
    struct ProjectConfig;
    class Window;
    class InputManager;
    class ResourceManager;
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
}

namespace ObSL {
    class Interpreter;
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
        ObSL::Interpreter *scriptEngine = nullptr;
        Sound::AudioEngine *audioEngine = nullptr;
        float deltaTime = 0.0f;
        std::shared_ptr<Project> activeProject = nullptr;
    };
} // namespace Core
