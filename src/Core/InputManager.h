#ifndef OBLIBERRY_INPUTMANAGER_H
#define OBLIBERRY_INPUTMANAGER_H

#include <GLFW/glfw3.h>

class InputManager {
public:
    void BeginFrame();

    void HandleKeyEvent(int key, int action);

    void HandleScrollEvent(double xOffset, double yOffset);

    void HandleClickEvent(int button, int action, int mods);

    void SetMousePos(double xPos, double yPos);

    double mousePosX = 0.0;
    double mousePosY = 0.0;

    double scrollX = 0.0;
    double scrollY = 0.0;

    bool IsKeyDown(int key) const;

    bool IsKeyPressed(int key) const;

    bool IsKeyReleased(int key) const;

    bool IsMouseDown(int button) const;

    bool IsMousePressed(int button) const;

    bool IsMouseReleased(int button) const;

private:
    bool keys[GLFW_KEY_LAST + 1]{};
    bool previousKeys[GLFW_KEY_LAST + 1]{};

    bool mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1]{};
    bool previousMouseButtons[GLFW_MOUSE_BUTTON_LAST + 1]{};

    static bool IsValidKey(int key);

    static bool IsValidMouseButton(int button);
};

#endif //OBLIBERRY_INPUTMANAGER_H
