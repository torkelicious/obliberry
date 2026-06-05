#ifndef ISOMETRICGAME_APPLICATION_H
#define ISOMETRICGAME_APPLICATION_H

#include "Window.h"
#include "Game/GameWorld.h"
#include "InputManager.h"

class Application {
public:
    Application();

    void Run();

    void Shutdown();

private:
    InputManager m_InputManager;
    Window m_Window;
    GameWorld m_GameWorld;
};

#endif //ISOMETRICGAME_APPLICATION_H
