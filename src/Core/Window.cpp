#include "Window.h"
#include <iostream>

#include "Graphics/GLDebug.h"

#include "InputManager.h" // include here (not in Window.h)

Window::Window(unsigned int width, unsigned int height, const char *title, int GLDebug)
    : EnableDebug(GLDebug) {
    if (!Init(width, height, title)) {
        throw std::runtime_error("Failed to initialize window");
    }
}

Window::~Window() {
    delete m_InputManager;
    m_InputManager = nullptr;

    if (m_Window) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    glfwTerminate();
}

void Window::PollEvents() {
    glfwPollEvents();
}

void Window::SwapBuffers() {
    glfwSwapBuffers(m_Window);
}

bool Window::Init(unsigned int width, unsigned int height, const char *title) {
    m_Width = width;
    m_Height = height;

    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, EnableDebug);

    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_Window) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_Window);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "failed to init glad" << std::endl;
        return false; // don't return -1 from a bool function
    }

    glViewport(0, 0, width, height);
    glfwSwapInterval(1);

    glfwSetWindowUserPointer(m_Window, this);

    glfwSetFramebufferSizeCallback(m_Window, WindowResizeCallback);

    m_InputManager = new InputManager();
    glfwSetKeyCallback(m_Window, KeyCallback);
    glfwSetCursorPosCallback(m_Window, CursorPosCallback);

    return true;
}

void Window::WindowResizeCallback(GLFWwindow *window, int width, int height) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self) return;

    self->m_Width = width;
    self->m_Height = height;
    glViewport(0, 0, width, height);

    if (self->m_Camera) {
        float aspect = (float) width / (float) height;
        self->m_Camera->SetAspect(aspect);
    }
}

void Window::SetCamera(Camera *camera) {
    m_Camera = camera;
}

void Window::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self || !self->m_InputManager) return;
    GLFWKeyPress input{key, scancode, action, mods};
    self->m_InputManager->HandleKey(input, window);
}

void Window::CursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self || !self->m_InputManager) return;

    self->m_InputManager->HandleMouseMove(xpos, ypos, window);
}
