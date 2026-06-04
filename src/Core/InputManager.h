#ifndef ISOMETRICGAME_INPUTMANAGER_H
#define ISOMETRICGAME_INPUTMANAGER_H
#include "Game/GameWorld.h"

struct GLFWwindow; // forward declaration (from GLFW)

struct GLFWKeyPress {
    int key;
    int scancode;
    int action;
    int mods;
};

class InputManager {
public:
    void HandleKey(const GLFWKeyPress &input, GLFWwindow *win);

    void HandleMouseMove(const double xpos, const double ypos, GLFWwindow *win);

    void SetGameWorld(GameWorld& gameWorld) {
        m_GameWorld = &gameWorld;
    }

private:
    GameWorld* m_GameWorld = nullptr;
    void MovePlayer(glm::vec2 mov);
};

#endif //ISOMETRICGAME_INPUTMANAGER_H
