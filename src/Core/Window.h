#ifndef OBLIBERRY_WINDOW_H
#define OBLIBERRY_WINDOW_H
#include <GLFW/glfw3.h>

#include "InputManager.h"

class Window {
public:
    Window(unsigned int width, unsigned int height, const char *title);

    ~Window();

    Window(const Window &) = delete;

    Window &operator=(const Window &) = delete;

    static void PollEvents();

    void SwapBuffers() const;

    [[nodiscard]] bool ShouldClose() const { return glfwWindowShouldClose(m_Window); }

    void Close() const { glfwSetWindowShouldClose(m_Window, true); }

    [[nodiscard]] int GetWidth() const { return m_Width; }
    [[nodiscard]] int GetHeight() const { return m_Height; }

    [[nodiscard]] GLFWwindow *GetNativeWindow() const { return m_Window; }

    void SetInputManager(InputManager *inputManager);

    void SetWindowTitle(const std::string &title) const;

private:
    GLFWwindow *m_Window = nullptr;
    InputManager *m_InputManager = nullptr;
    int m_Width = 0;
    int m_Height = 0;

    bool Init(unsigned int width, unsigned int height, const char *title);

    static void WindowResizeCallback(GLFWwindow *window, int width, int height);

    static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

    static void CursorPosCallback(GLFWwindow *window, double xpos, double ypos);

    static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

    static void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
};


#endif //OBLIBERRY_WINDOW_H
