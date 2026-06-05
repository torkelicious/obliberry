#ifndef ISOMETRICGAME_PLAYERINPUTCOMPONENT_H
#define ISOMETRICGAME_PLAYERINPUTCOMPONENT_H
#include <GLFW/glfw3.h>

#include "ECS/ECS.h"

struct PlayerInput : public Component {
    int Up = GLFW_KEY_W;
    int Down = GLFW_KEY_S;
    int Left = GLFW_KEY_A;
    int Right = GLFW_KEY_D;
};

#endif //ISOMETRICGAME_PLAYERINPUTCOMPONENT_H
