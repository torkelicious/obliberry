#include "InputManager.h"

#include <iostream>
#include <GLFW/glfw3.h>

void InputManager::HandleKey(const GLFWKeyPress &input, GLFWwindow *win) {
    switch (input.key) {
        case GLFW_KEY_ESCAPE:
            if (input.action == GLFW_PRESS)glfwSetWindowShouldClose(win, GLFW_TRUE);
            break;
    }
}

void InputManager::HandleMouseMove(const double xpos, const double ypos, GLFWwindow *win) {
    //std::cout << "Mouse: " << "X: " << xpos << " Y: " << ypos << std::endl;
}
