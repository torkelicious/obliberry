#pragma once
#include "Config/GraphicsConfig.h"
#include "Logger/Logger.h"
#include "UI/Rendering/UIRenderer.h"
#include "UI/Rendering/UISystem.h"

#include <filesystem>
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
        std::string pendingScenePath; // for deferred scene loading (GameLayer, Scripting)
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
    };
} // namespace Core
