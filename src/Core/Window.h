#ifndef ISOMETRICGAME_WINDOW_H
#define ISOMETRICGAME_WINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Application;
class Camera;
class InputManager;

class Window {
public:
    Window(unsigned int width, unsigned int height, const char *title, int GLDebug, Application *app);

    ~Window();

    Window(const Window &) = delete;

    Window &operator=(const Window &) = delete;

    void PollEvents();

    void SwapBuffers();

    bool ShouldClose() const { return glfwWindowShouldClose(m_Window); }

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

    GLFWwindow *GetNativeWindow() const { return m_Window; }

    void SetCamera(Camera *camera);

    void SetInputManager(InputManager *inputManager);

    void Shutdown();

private:
    GLFWwindow *m_Window = nullptr;
    Camera *m_Camera = nullptr;
    InputManager *m_InputManager = nullptr;
    Application *m_App = nullptr;

    int m_Width = 0;
    int m_Height = 0;

    bool Init(unsigned int width, unsigned int height, const char *title);

    static void WindowResizeCallback(GLFWwindow *window, int width, int height);

    static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);


    static void CursorPosCallback(GLFWwindow *window, double xpos, double ypos);

    const int EnableDebug;
};

#endif //ISOMETRICGAME_WINDOW_H
