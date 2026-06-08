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
    inline void Update(std::span<const Entity> entities, const InputManager &inputManager) {
        for (const Entity entity: entities) {
            auto *inputComp = entity.GetComponent<PlayerInputComponent>();
            if (!inputComp) continue;

            if (inputManager.IsMousePressed(inputComp->LeftClick)) {
                std::cout << "system: mouse clicked\n";
            }
        }
    }
}


#endif //OBLIBERRY_PLAYERINPUT_H
