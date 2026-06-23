#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdexcept>
#include "Window.h"
#include "Constants.h"
#include "imgui.h"

Window::Window(const unsigned int width, const unsigned int height, const char *title, const bool fullscreen) {
    if (!Init(width, height, title, fullscreen)) {
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

bool Window::Init(const unsigned int width, const unsigned int height, const char *title, const bool fullscreen) {
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

    GLFWmonitor *monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    m_Window = glfwCreateWindow(m_Width, m_Height, title, monitor, nullptr);

    if (!m_Window) {
        glfwTerminate();
        return false;
    }

    glfwGetWindowSize(m_Window, &m_Width, &m_Height);

    glfwMakeContextCurrent(m_Window);
    glfwSetWindowUserPointer(m_Window, this);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        return false;
    }

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_Window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    glfwSetFramebufferSizeCallback(m_Window, WindowResizeCallback);
    glfwSetKeyCallback(m_Window, KeyCallback);
    glfwSetCursorPosCallback(m_Window, CursorPosCallback);
    glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
    glfwSetScrollCallback(m_Window, ScrollCallback);

    return true;
}

void Window::WindowResizeCallback(GLFWwindow *window, const int width, const int height) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self) return;

    self->m_Width = width;
    self->m_Height = height;

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
}

void Window::SetInputManager(InputManager *inputManager) {
    m_InputManager = inputManager;
}

void Window::SetWindowTitle(const std::string &title) const {
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

    if (self->m_InputManager) {
        self->m_InputManager->HandleScrollEvent(xoffset, yoffset);
    }
}

void Window::SetFullscreen(const bool fullscreen) const {
    if (fullscreen) {
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_Window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(m_Window, nullptr, 100, 100, m_Width, m_Height, 0);
    }
}
