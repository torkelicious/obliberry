#ifndef OBLIBERRY_APPLICATION_H
#define OBLIBERRY_APPLICATION_H
#include "ResourceManager.h"
#include "Window.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"


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
};


#endif //OBLIBERRY_APPLICATION_H
