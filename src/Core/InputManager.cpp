#include "InputManager.h"
#include <algorithm>

void InputManager::BeginFrame() {
    std::copy_n(keys, GLFW_KEY_LAST + 1, previousKeys);
    //scrollX = 0.0;
    //scrollY = 0.0;
}

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

void InputManager::HandleScrollEvent(double xOffset, double yOffset) {
    scrollX += xOffset;
    scrollY += yOffset;
}
