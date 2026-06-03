#ifndef ISOMETRICGAME_WINDOW_H
#define ISOMETRICGAME_WINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Graphics/Camera.h"

class InputManager; // forward declaration

class Window {
public:
    explicit Window(
        unsigned int width,
        unsigned int height,
        const char *title,
        const int GLDebug = false
    );

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

private:
    GLFWwindow *m_Window = nullptr;
    Camera *m_Camera = nullptr;
    InputManager *m_InputManager = nullptr;

    int m_Width = 0;
    int m_Height = 0;

    bool Init(unsigned int width, unsigned int height, const char *title);

    static void WindowResizeCallback(GLFWwindow *window, int width, int height);

    static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

    static void CursorPosCallback(GLFWwindow *window, double xpos, double ypos);

    const int EnableDebug;
};

#endif //ISOMETRICGAME_WINDOW_H
