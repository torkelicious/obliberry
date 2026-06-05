#include <iostream>
#include <stdexcept>
#include "Window.h"
#include "Application.h"


Window::Window(unsigned int width, unsigned int height, const char *title, int GLDebug, Application *app)
    : EnableDebug(GLDebug) {
    if (!Init(width, height, title)) {
        throw std::runtime_error("Failed to initialize window");
    }
    m_App = app;
}

Window::~Window() {
    glfwDestroyWindow(m_Window);
    m_Window = nullptr;
    glfwTerminate();
}

void Window::PollEvents() {
    glfwPollEvents();
}

void Window::SwapBuffers() {
    glfwSwapBuffers(m_Window);
}

bool Window::Init(unsigned int width, unsigned int height, const char *title) {
    m_Width = static_cast<int>(width);
    m_Height = static_cast<int>(height);

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

void Window::SetInputManager(InputManager *inputManager) {
    m_InputManager = inputManager;
}


void Window::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void) scancode;
    (void) mods;

    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self) return;

    if (self->m_InputManager) {
        self->m_InputManager->HandleKeyEvent(key, action);
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        //        glfwSetWindowShouldClose(window, GLFW_TRUE);
        self->m_App->Shutdown();
    }
}

void Window::CursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
    // TODO: implement
}


void Window::Shutdown() {
    if (m_Window) {
        glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
        //glfwDestroyWindow(m_Window);
        //m_Window = nullptr;
    }
    //glfwTerminate();
}
