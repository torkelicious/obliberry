#ifndef OBLIBERRY_PLAYERINPUT_H
#define OBLIBERRY_PLAYERINPUT_H

#include <span>
#include <iostream>
#include <glm/glm.hpp>
#include "Core/InputManager.h"
#include "ECS/Components/PlayerInputComponent.h"
#include "ECS/ECS.h"

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
