#include "Window.h"
#include <iostream>

Window::Window(unsigned int width, unsigned int height, const char *title) {
    if (!Init(width, height, title)) {
        throw std::runtime_error(
            "Failed to initialize window");
        // todo: add error handling later
    }
}

Window::~Window() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
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
    if (!glfwInit()) {
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    if (!m_Window) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_Window);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "failed to init glad" << std::endl;
        return -1;
    }

    glViewport(0, 0, width, height);
    glfwSwapInterval(1); //vsync

    glfwSetWindowUserPointer(m_Window, this);

    glfwSetFramebufferSizeCallback(m_Window, WindowResizeCallback);
    glfwSetKeyCallback(m_Window, KeyCallback);

    return true;
}

void Window::WindowResizeCallback(
    GLFWwindow *window,
    int width,
    int height) {
    auto *self =
            static_cast<Window *>(
                glfwGetWindowUserPointer(window));

    self->m_Width = width;
    self->m_Height = height;

    glViewport(0, 0, width, height);
}


void Window::KeyCallback(
    GLFWwindow *window,
    int key,
    int scancode,
    int action,
    int mods) {
    if (key == GLFW_KEY_ESCAPE &&
        action == GLFW_PRESS) {
        glfwSetWindowShouldClose(
            window,
            GLFW_TRUE);
    }
}