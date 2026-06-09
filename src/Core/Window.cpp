#include <iostream>
#include <stdexcept>
#include "Window.h"

#include "imgui.h"
#include "Renderer/GLDebug.h"

Window::Window(unsigned int width, unsigned int height, const char *title) {
    if (!Init(width, height, title)) {
        throw std::runtime_error("Failed to initialize window");
    }
}

Window::~Window() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    if (glfwGetCurrentContext()) {
        glfwTerminate();
    }
}


void Window::PollEvents() {
    glfwPollEvents();
}

void Window::SwapBuffers() {
    glfwSwapBuffers(m_Window);
}

bool Window::Init(unsigned int width, unsigned int height, const char *title) {
    /*
     * TODO: window resizing causes temporary stuttering/freezes
     *   seems to occur specifically on NVIDIA (Proprietary Drivers) on KWin,
    */

    m_Width = static_cast<float>(width);
    m_Height = static_cast<float>(height);

    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_Window) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_Window);


    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "failed to init glad" << std::endl;
        return false;
    }

    int dbg = GLDebug::InitDebug();
    dbg == 0
        ? std::cout << "Debug Enabled" << std::endl
        : std::cout << "Debug unavailable" << std::endl;

    glViewport(0, 0, width, height);
    glfwSwapInterval(1);

    glfwSetWindowUserPointer(m_Window, this);

    glfwSetFramebufferSizeCallback(m_Window, WindowResizeCallback);

    glfwSetKeyCallback(m_Window, KeyCallback);
    glfwSetCursorPosCallback(m_Window, CursorPosCallback);
    glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
    glfwSetScrollCallback(m_Window, ScrollCallback);

    return true;
}

void Window::WindowResizeCallback(GLFWwindow *window, int width, int height) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (self) {
        self->m_Width = static_cast<float>(width);
        self->m_Height = static_cast<float>(height);
    }

    float targetAspect = 16.0f / 9.0f;
    float windowAspect = (float) width / height;
    int viewportWidth;
    int viewportHeight;
    int viewportX;
    int viewportY;

    if (windowAspect > targetAspect) {
        viewportHeight = height;
        viewportWidth = (int) (height * targetAspect);
        viewportX = (width - viewportWidth) / 2;
        viewportY = 0;
    } else {
        viewportWidth = width;
        viewportHeight = (int) (width / targetAspect);
        viewportX = 0;
        viewportY = (height - viewportHeight) / 2;
    }
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
}

void Window::SetInputManager(InputManager *inputManager) {
    m_InputManager = inputManager;
}

void Window::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self) return;

    if (self->m_InputManager) {
        self->m_InputManager->HandleKeyEvent(key, action);
    }
}

void Window::CursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self) return;

    if (self->m_InputManager) {
        self->m_InputManager->SetMousePos(xpos, ypos);
    }
}

void Window::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self) return;

    if (self->m_InputManager) {
        self->m_InputManager->HandleClickEvent(button, action, mods);
    }
}

void Window::ScrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self) return;
    self->m_InputManager->HandleScrollEvent(xoffset, yoffset);
}
