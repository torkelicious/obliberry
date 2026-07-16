#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include "Logger/LoggerService.h"
#include "Rendering/GLDebug.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <nfd_glfw3.h>
#include <stdexcept>
#include "Window.h"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "Window"

Platform::Window::Window::Window(const unsigned int width, const unsigned int height, const char *title, const Config::GraphicsConfig *graphicsConf) {
    if (!Init(width, height, title, graphicsConf)) {
        throw std::runtime_error("Failed to initialize window");
    }
}

Platform::Window::Window::~Window() {
    NFD_Quit();
    if (m_Window) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    if (glfwGetCurrentContext()) {
        glfwTerminate();
    }
}

void Platform::Window::Window::PollEvents() { glfwPollEvents(); }

void Platform::Window::Window::SwapBuffers() const { glfwSwapBuffers(m_Window); }

bool Platform::Window::Window::Init(const unsigned int width, const unsigned int height, const char *title, const Config::GraphicsConfig *conf) {
    m_Width = static_cast<int>(width);
    m_Height = static_cast<int>(height);


    if (!glfwInit()) {
        LOG_ERROR(LOG_WHO, "Failed to initalize GLFW");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if (conf->MSAAEnabled) {
        glfwWindowHint(GLFW_SAMPLES, conf->AASamples); // MSAA
    }

#if defined DEBUG_BUILD
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

    GLFWmonitor *monitor = conf->Fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    m_Window = glfwCreateWindow(m_Width, m_Height, title, monitor, nullptr);

    if (!m_Window) {
        LOG_ERROR(LOG_WHO, "Failed to create native Window");
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, WindowResizeCallback);

    glfwGetWindowSize(m_Window, &m_Width, &m_Height);


    if (s_ShouldInitNFD) {
        LOG_INFO(LOG_WHO, "Initializing NFD-E");
        if (NFD_Init() != NFD_OKAY) {
            LOG_WARN(LOG_WHO, "Failed to initialize Native File Dialog");
        }
// wont do anything on nonlinux/nonwayland displays anyways
#if defined(__linux__)
        NFD_SetDisplayPropertiesFromGLFW();
#endif
    }

    glfwMakeContextCurrent(m_Window);
    glfwSetWindowUserPointer(m_Window, this);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        LOG_ERROR(LOG_WHO, "Failed to initalize GLAD");
        return false;
    }

#if defined DEBUG_BUILD
    // must come after Glad INIT
    Rendering::GLDebug::InitDebug();
#endif

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_Window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    glfwSetKeyCallback(m_Window, KeyCallback);
    glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
    glfwSetCursorPosCallback(m_Window, CursorPosCallback);
    glfwSetScrollCallback(m_Window, ScrollCallback);

    return true;
}

void Platform::Window::Window::WindowResizeCallback(GLFWwindow *window, const int width, const int height) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self)
        return;

    self->m_Width = width;
    self->m_Height = height;

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
}

void Platform::Window::Window::SetInputManager(Platform::Input::InputManager *inputManager) { m_InputManager = inputManager; }

void Platform::Window::Window::SetWindowTitle(const std::string &title) const { glfwSetWindowTitle(m_Window, title.c_str()); }

void Platform::Window::Window::KeyCallback(GLFWwindow *window, const int key, int scancode, const int action, int mods) {
    const auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self)
        return;

    if (self->m_InputManager) {
        self->m_InputManager->HandleKeyEvent(key, action);
    }
}

void Platform::Window::Window::CursorPosCallback(GLFWwindow *window, const double xpos, const double ypos) {
    const auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self)
        return;

    if (self->m_InputManager) {
        self->m_InputManager->SetMousePos(xpos, ypos);
    }
}

void Platform::Window::Window::MouseButtonCallback(GLFWwindow *window, const int button, const int action, const int mods) {
    const auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self)
        return;

    if (self->m_InputManager) {
        self->m_InputManager->HandleClickEvent(button, action, mods);
    }
}

void Platform::Window::Window::ScrollCallback(GLFWwindow *window, const double xoffset, const double yoffset) {
    const auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self)
        return;

    if (self->m_InputManager) {
        self->m_InputManager->HandleScrollEvent(xoffset, yoffset);
    }
}

void Platform::Window::Window::SetFullscreen(const bool fullscreen) const {
    if (fullscreen) {
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_Window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(m_Window, nullptr, 100, 100, m_Width, m_Height, 0);
    }
}
#pragma pop_macro("LOG_WHO")
