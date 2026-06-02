#ifndef ISOMETRICGAME_APPLICATION_H
#define ISOMETRICGAME_APPLICATION_H

#include "Window.h"

class Application {
public:
    Application();
    void Run();
private:
    // app window instantiated with class
    Window m_Window;
};

#endif //ISOMETRICGAME_APPLICATION_H
