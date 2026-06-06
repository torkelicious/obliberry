#include "InputManager.h"
#include <algorithm>
#include <iostream>

// FRAME UPDATE

void InputManager::BeginFrame() {
    std::copy_n(keys, GLFW_KEY_LAST + 1, previousKeys);
    std::copy_n(mouseButtons, GLFW_MOUSE_BUTTON_LAST + 1, previousMouseButtons);

    scrollX = 0.0;
    scrollY = 0.0;
}

// KEY INPUT

bool InputManager::IsValidKey(int key) {
    return key >= 0 && key <= GLFW_KEY_LAST;
}

void InputManager::HandleKeyEvent(int key, int action) {
    if (!IsValidKey(key)) return;
    keys[key] = action != GLFW_RELEASE;
}

bool InputManager::IsKeyDown(int key) const {
    return IsValidKey(key) && keys[key];
}

bool InputManager::IsKeyPressed(int key) const {
    return IsValidKey(key) && keys[key] && !previousKeys[key];
}

bool InputManager::IsKeyReleased(int key) const {
    return IsValidKey(key) && !keys[key] && previousKeys[key];
}

// MOUSE INPUT

bool InputManager::IsValidMouseButton(int button) {
    return button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST;
}

void InputManager::HandleClickEvent(int button, int action, int mods) {
    if (!IsValidMouseButton(button)) return;
    mouseButtons[button] = action != GLFW_RELEASE;
}

bool InputManager::IsMouseDown(int button) const {
    return IsValidMouseButton(button) && mouseButtons[button];
}

bool InputManager::IsMousePressed(int button) const {
    return IsValidMouseButton(button) &&
           mouseButtons[button] &&
           !previousMouseButtons[button];
}

bool InputManager::IsMouseReleased(int button) const {
    return IsValidMouseButton(button) &&
           !mouseButtons[button] &&
           previousMouseButtons[button];
}

// MOUSE POSITION

void InputManager::SetMousePos(double xPos, double yPos) {
    mousePosX = xPos;
    mousePosY = yPos;
}

// SCROLL

void InputManager::HandleScrollEvent(double xOffset, double yOffset) {
    scrollX += xOffset;
    scrollY += yOffset;
}
