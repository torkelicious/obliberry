#ifndef ISOMETRICGAME_INPUTSYSTEM_H
#define ISOMETRICGAME_INPUTSYSTEM_H

#include <cmath>
#include <glm/glm.hpp>
#include "Core/InputManager.h"
#include "ECS/ECS.h"
#include "ECS/Components/PlayerInputComponent.h"
#include "ECS/Components/VelocityComponent.h"

class InputSystem {
public:
    explicit InputSystem(const InputManager &inputManager)
        : m_InputManager(&inputManager) {
    }

    void SetInputManager(const InputManager &inputManager) {
        m_InputManager = &inputManager;
    }

    void Update(Entity &entity) const {
        if (!m_InputManager) return;

        auto *playerInput = entity.GetComponent<PlayerInput>();
        auto *velocity = entity.GetComponent<Velocity>();
        if (!playerInput || !velocity) return;

        glm::vec2 movement{0.0f, 0.0f};

        glm::vec2 scroll = {
            m_InputManager->scrollX,
            m_InputManager->scrollY
        };

        if (m_InputManager->IsKeyDown(playerInput->Up)) {
            movement.y += 1.0f;
        }
        if (m_InputManager->IsKeyDown(playerInput->Down)) {
            movement.y -= 1.0f;
        }
        if (m_InputManager->IsKeyDown(playerInput->Left)) {
            movement.x -= 1.0f;
        }
        if (m_InputManager->IsKeyDown(playerInput->Right)) {
            movement.x += 1.0f;
        }

        const float lengthSquared = movement.x * movement.x + movement.y * movement.y;
        if (lengthSquared > 0.0f) {
            movement /= std::sqrt(lengthSquared);
        }

        velocity->Value = movement;
    }

private:
    const InputManager *m_InputManager = nullptr;
};

#endif //ISOMETRICGAME_INPUTSYSTEM_H


