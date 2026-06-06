#ifndef OBLIBERRY_PLAYERINPUT_H
#define OBLIBERRY_PLAYERINPUT_H
#include <glm/glm.hpp>
#include "Core/InputManager.h"
#include "ECS/ECS.h"
#include "ECS/Components/VelocityComponent.h"

struct PlayerInput : public Component {
    int LeftClick = GLFW_MOUSE_BUTTON_LEFT;
    int RightClick = GLFW_MOUSE_BUTTON_RIGHT;
    int Up = GLFW_KEY_W;
    int Down = GLFW_KEY_S;
    int Left = GLFW_KEY_A;
    int Right = GLFW_KEY_D;
};

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

        glm::vec2 MousePos = {
            m_InputManager->mousePosX,
            m_InputManager->mousePosY,
        };

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

        if (m_InputManager->IsMousePressed(playerInput->LeftClick)) {
            std::cout << "system: mouse clicked" << std::endl;
        }

        velocity->Value = movement;
    }

private:
    const InputManager *m_InputManager = nullptr;
};


#endif //OBLIBERRY_PLAYERINPUT_H
