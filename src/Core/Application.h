#ifndef ISOMETRICGAME_APPLICATION_H
#define ISOMETRICGAME_APPLICATION_H

#include "Window.h"
#include "Game/GameWorld.h"

class Application {
public:
    Application();

    void Run();

private:
    Window m_Window;
    GameWorld m_GameWorld;
};

#endif //ISOMETRICGAME_APPLICATION_H
