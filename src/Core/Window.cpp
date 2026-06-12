#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <iostream>
#include <stdexcept>
#include "Window.h"

#include "Constants.h"
#include "imgui.h"
#include "Renderer/GLDebug.h"

Window::Window(const unsigned int width, const unsigned int height, const char *title) {
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

void Window::SwapBuffers() const {
    glfwSwapBuffers(m_Window);
}

bool Window::Init(const unsigned int width, const unsigned int height, const char *title) {
    /*
     * TODO: window resizing causes temporary stuttering/freezes
     *   seems to occur specifically on NVIDIA (Proprietary Drivers) on KWin,
    */

    m_Width = static_cast<int>(width);
    m_Height = static_cast<int>(height);

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


    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "failed to init glad" << std::endl;
        return false;
    }

    const int dbg = GLDebug::InitDebug();
    dbg == 0
        ? std::cout << "Debug Enabled" << std::endl
        : std::cout << "Debug unavailable" << std::endl;

    glViewport(0, 0, width, height);

    glfwSetWindowUserPointer(m_Window, this);

    glfwSetFramebufferSizeCallback(m_Window, WindowResizeCallback);

    glfwSetKeyCallback(m_Window, KeyCallback);
    glfwSetCursorPosCallback(m_Window, CursorPosCallback);
    glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
    glfwSetScrollCallback(m_Window, ScrollCallback);

    return true;
}

void Window::WindowResizeCallback(GLFWwindow *window, const int width, const int height) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (self) {
        self->m_Width = width;
        self->m_Height = height;
    }

    const float windowAspect = static_cast<float>(width) / height;
    int viewportWidth;
    int viewportHeight;
    int viewportX;
    int viewportY;

    if (windowAspect > TARGET_ASPECT) {
        viewportHeight = height;
        viewportWidth = static_cast<int>(height * TARGET_ASPECT);
        viewportX = (width - viewportWidth) / 2;
        viewportY = 0;
    } else {
        viewportWidth = width;
        viewportHeight = static_cast<int>(width / TARGET_ASPECT);
        viewportX = 0;
        viewportY = (height - viewportHeight) / 2;
    }
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
}

void Window::SetInputManager(InputManager *inputManager) {
    m_InputManager = inputManager;
}

void Window::SetWindowTitle(const std::string &title) {
    glfwSetWindowTitle(m_Window, title.c_str());
}

void Window::KeyCallback(GLFWwindow *window, const int key, int scancode, const int action, int mods) {
    const auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self) return;

    if (self->m_InputManager) {
        self->m_InputManager->HandleKeyEvent(key, action);
    }
}

void Window::CursorPosCallback(GLFWwindow *window, const double xpos, const double ypos) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    const auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self) return;

    if (self->m_InputManager) {
        self->m_InputManager->SetMousePos(xpos, ypos);
    }
}

void Window::MouseButtonCallback(GLFWwindow *window, const int button, const int action, const int mods) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    const auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self) return;

    if (self->m_InputManager) {
        self->m_InputManager->HandleClickEvent(button, action, mods);
    }
}

void Window::ScrollCallback(GLFWwindow *window, const double xoffset, const double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    const auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self) return;
    self->m_InputManager->HandleScrollEvent(xoffset, yoffset);
}
