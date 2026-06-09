#ifndef OBLIBERRY_PLAYERINPUT_H
#define OBLIBERRY_PLAYERINPUT_H

#include <span>
#include <iostream>
#include <glm/glm.hpp>
#include "Core/InputManager.h"
#include "ECS/ECS.h"

struct PlayerInputComponent {
    int LeftClick = GLFW_MOUSE_BUTTON_LEFT;
    int RightClick = GLFW_MOUSE_BUTTON_RIGHT;
    int Up = GLFW_KEY_W;
    int Down = GLFW_KEY_S;
    int Left = GLFW_KEY_A;
    int Right = GLFW_KEY_D;
};

namespace InputSystem {
    inline void Update(Registry &registry, const InputManager &inputManager) {
        registry.ForEach<PlayerInputComponent>(
            [&](Entity entity, PlayerInputComponent *inputComp) {
                if (inputManager.IsMousePressed(inputComp->LeftClick)) {
                    std::cout << "system: mouse clicked\n";
                }
            }
        );
    }
}


#endif //OBLIBERRY_PLAYERINPUT_H
