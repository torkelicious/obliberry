#pragma once

#include "ResourceManager.h"
#include "Window.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"
#include "Sound/AudioEngine.h"


class Application {
public:
    Application();

    ~Application() {
        Shutdown();
    }

    void Run();

    static void Shutdown();

private:
    Window m_Window;
    InputManager m_InputManager;
    ResourceManager m_ResourceManager;
    ObSL::Interpreter m_ScriptEngine;
    std::unique_ptr<AudioEngine> m_AudioEngine;
};

