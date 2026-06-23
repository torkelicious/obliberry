#pragma once

#include "ProjectConfig.h"
#include "ResourceManager.h"
#include "Window.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"
#include "Sound/AudioEngine.h"


class Application {
public:
    explicit Application(ProjectConfig config);

    ~Application() {
        Shutdown();
    }

    void Run();

    static void Shutdown();

private:
    ProjectConfig m_Project;
    Window m_Window;
    InputManager m_InputManager;
    ResourceManager m_ResourceManager;
    ObSL::Interpreter m_ScriptEngine;
    std::unique_ptr<AudioEngine> m_AudioEngine;
};

