#include "InputManager.h"

#include <iostream>
#include <GLFW/glfw3.h>

void InputManager::HandleKey(const GLFWKeyPress &input, GLFWwindow *win) {
    switch (input.key) {
        case GLFW_KEY_ESCAPE:
            if (input.action == GLFW_PRESS)glfwSetWindowShouldClose(win, GLFW_TRUE);
            break;
        case GLFW_KEY_W:
        case GLFW_KEY_UP:
            MovePlayer({0,0.5});
            break;
        case GLFW_KEY_S:
        case GLFW_KEY_DOWN:
            MovePlayer({0,-0.5});
            break;
        case GLFW_KEY_A:
        case GLFW_KEY_LEFT:
            MovePlayer({-0.5,0});
            break;
        case GLFW_KEY_D:
        case GLFW_KEY_RIGHT:
            MovePlayer({0.5,0});
            break;
    }
}

void InputManager::HandleMouseMove(const double xpos, const double ypos, GLFWwindow *win) {
    //std::cout << "Mouse: " << "X: " << xpos << " Y: " << ypos << std::endl;
}

void InputManager::MovePlayer(glm::vec2 mov) {
    m_GameWorld->GetPlayer().move(mov);
}
