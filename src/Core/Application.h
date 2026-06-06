#ifndef OBLIBERRY_APPLICATION_H
#define OBLIBERRY_APPLICATION_H
#include "Window.h"


class Application {
public:
    Application();

    ~Application() {
        Shutdown();
    }

    void Run();

    void Shutdown();

private:
    Window m_Window;
    InputManager m_InputManager;
};


#endif //OBLIBERRY_APPLICATION_H
