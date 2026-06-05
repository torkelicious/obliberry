#ifndef ISOMETRICGAME_INPUTMANAGER_H
#define ISOMETRICGAME_INPUTMANAGER_H
#include <GLFW/glfw3.h>

class InputManager {
public:
    void BeginFrame();

    void HandleKeyEvent(int key, int action);

    [[nodiscard]] bool IsKeyDown(int key) const;

    [[nodiscard]] bool IsKeyPressed(int key) const;

    [[nodiscard]] bool IsKeyReleased(int key) const;

private:
    bool keys[GLFW_KEY_LAST + 1]{};
    bool previousKeys[GLFW_KEY_LAST + 1]{};

    static bool IsValidKey(int key);
};

#endif //ISOMETRICGAME_INPUTMANAGER_H
