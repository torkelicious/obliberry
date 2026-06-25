#pragma once

#include <string>
#include <GLFW/glfw3.h>

class InputManager {
public:
    void BeginFrame();

    void HandleKeyEvent(int key, int action);

    void HandleScrollEvent(double xOffset, double yOffset);

    void HandleClickEvent(int button, int action, int mods);

    void SetMousePos(double xPos, double yPos);

    static int GetKeyFromName(const std::string &keyName); // in KeyMappings.cpp

    [[nodiscard]] double MousePosX() const noexcept { return m_MousePosX; }
    [[nodiscard]] double MousePosY() const noexcept { return m_MousePosY; }
    [[nodiscard]] double ScrollX() const noexcept { return m_ScrollX; }
    [[nodiscard]] double ScrollY() const noexcept { return m_ScrollY; }

    [[nodiscard]] bool IsKeyDown(int key) const;
    [[nodiscard]] bool IsKeyDown(const std::string &keyAlias) const;

    [[nodiscard]] bool IsKeyPressed(int key) const;
    [[nodiscard]] bool IsKeyPressed(const std::string &KeyAlias) const;

    [[nodiscard]] bool IsKeyReleased(int key) const;
    [[nodiscard]] bool IsKeyReleased(const std::string &keyAlias) const;

    [[nodiscard]] bool IsMouseDown(int button) const;
    [[nodiscard]] bool IsMousePressed(int button) const;
    [[nodiscard]] bool IsMouseReleased(int button) const;

private:
    bool keys[GLFW_KEY_LAST + 1]{};
    bool previousKeys[GLFW_KEY_LAST + 1]{};

    bool mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1]{};
    bool previousMouseButtons[GLFW_MOUSE_BUTTON_LAST + 1]{};

    static bool IsValidKey(int key);

    static bool IsValidMouseButton(int button);

    double m_MousePosX = 0.0;
    double m_MousePosY = 0.0;

    double m_ScrollX = 0.0;
    double m_ScrollY = 0.0;
};
