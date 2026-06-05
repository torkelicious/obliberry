#include "InputManager.h"

#include <algorithm>

void InputManager::BeginFrame() {
    std::copy_n(keys, GLFW_KEY_LAST + 1, previousKeys);
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
