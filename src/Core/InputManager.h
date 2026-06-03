#ifndef ISOMETRICGAME_INPUTMANAGER_H
#define ISOMETRICGAME_INPUTMANAGER_H

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
};

#endif //ISOMETRICGAME_INPUTMANAGER_H
