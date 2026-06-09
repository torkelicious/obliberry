#ifndef OBLIBERRY_PLAYERINPUTCOMPONENT_H
#define OBLIBERRY_PLAYERINPUTCOMPONENT_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

struct PlayerInputComponent {
    int LeftClick = GLFW_MOUSE_BUTTON_LEFT;
    int RightClick = GLFW_MOUSE_BUTTON_RIGHT;
    int Up = GLFW_KEY_W;
    int Down = GLFW_KEY_S;
    int Left = GLFW_KEY_A;
    int Right = GLFW_KEY_D;
    int Quit = GLFW_KEY_ESCAPE;
};


#endif //OBLIBERRY_PLAYERINPUTCOMPONENT_H
