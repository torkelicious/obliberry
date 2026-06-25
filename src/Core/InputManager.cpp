#include "InputManager.h"
#include <algorithm>

void InputManager::BeginFrame() {
    std::copy_n(keys, GLFW_KEY_LAST + 1, previousKeys);
    std::copy_n(mouseButtons, GLFW_MOUSE_BUTTON_LAST + 1, previousMouseButtons);

    m_ScrollX = 0.0;
    m_ScrollY = 0.0;
}

bool InputManager::IsValidKey(const int key) {
    return key >= 0 && key <= GLFW_KEY_LAST;
}

void InputManager::HandleKeyEvent(const int key, const int action) {
    if (!IsValidKey(key)) return;
    keys[key] = action != GLFW_RELEASE;
}

bool InputManager::IsKeyDown(const int key) const {
    return IsValidKey(key) && keys[key];
}

bool InputManager::IsKeyDown(const std::string &keyAlias) const {
    return IsKeyDown(GetKeyFromName(keyAlias));
}

bool InputManager::IsKeyPressed(const int key) const {
    return IsValidKey(key) && keys[key] && !previousKeys[key];
}

bool InputManager::IsKeyPressed(const std::string &KeyAlias) const {
    return IsKeyPressed(GetKeyFromName(KeyAlias));
}

bool InputManager::IsKeyReleased(const int key) const {
    return IsValidKey(key) && !keys[key] && previousKeys[key];
}

bool InputManager::IsKeyReleased(const std::string &keyAlias) const {
    return IsKeyReleased(GetKeyFromName(keyAlias));
}

bool InputManager::IsValidMouseButton(const int button) {
    return button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST;
}

void InputManager::HandleClickEvent(const int button, const int action, int mods) {
    if (!IsValidMouseButton(button)) return;
    mouseButtons[button] = action != GLFW_RELEASE;
}

bool InputManager::IsMouseDown(const int button) const {
    return IsValidMouseButton(button) && mouseButtons[button];
}

bool InputManager::IsMousePressed(const int button) const {
    return IsValidMouseButton(button) &&
           mouseButtons[button] &&
           !previousMouseButtons[button];
}

bool InputManager::IsMouseReleased(const int button) const {
    return IsValidMouseButton(button) &&
           !mouseButtons[button] &&
           previousMouseButtons[button];
}

void InputManager::SetMousePos(const double xPos, const double yPos) {
    m_MousePosX = xPos;
    m_MousePosY = yPos;
}

void InputManager::HandleScrollEvent(const double xOffset, const double yOffset) {
    m_ScrollX += xOffset;
    m_ScrollY += yOffset;
}

// key mappings (GetKeyFromName) are in KeyMappings.cpp
