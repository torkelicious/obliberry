#include "InputManager.h"
#include <ranges>

#include <algorithm>

void Core::InputManager::BeginFrame() {
    std::copy_n(keys, GLFW_KEY_LAST + 1, previousKeys);
    std::copy_n(mouseButtons, GLFW_MOUSE_BUTTON_LAST + 1, previousMouseButtons);

    m_ScrollX = 0.0;
    m_ScrollY = 0.0;

    m_MouseDeltaX = 0.0;
    m_MouseDeltaY = 0.0;
}

//
// Keyboard
//


bool Core::InputManager::IsValidKey(const int key) { return key >= 0 && key <= GLFW_KEY_LAST; }

void Core::InputManager::HandleKeyEvent(const int key, const int action) {
    if (!IsValidKey(key))
        return;
    keys[key] = action != GLFW_RELEASE;
}

bool Core::InputManager::IsKeyDown(const int key) const { return IsValidKey(key) && keys[key]; }

bool Core::InputManager::IsKeyDown(const std::string &keyAlias) const { return IsKeyDown(GetKeyFromName(keyAlias)); }

bool Core::InputManager::IsKeyPressed(const int key) const {
    return IsValidKey(key) && keys[key] && !previousKeys[key];
}

bool Core::InputManager::IsKeyPressed(const std::string &KeyAlias) const {
    return IsKeyPressed(GetKeyFromName(KeyAlias));
}

bool Core::InputManager::IsKeyReleased(const int key) const {
    return IsValidKey(key) && !keys[key] && previousKeys[key];
}

bool Core::InputManager::IsKeyReleased(const std::string &keyAlias) const {
    return IsKeyReleased(GetKeyFromName(keyAlias));
}

bool Core::InputManager::IsKeyComboDown(const std::vector<std::string> &keyAliases) const {
    if (keyAliases.empty())
        return false;

    return std::ranges::all_of(keyAliases, [this](const std::string &alias) { return IsKeyDown(alias); });
}

bool Core::InputManager::IsKeyComboPressed(const std::vector<std::string> &keyAliases) const {
    if (keyAliases.empty())
        return false;

    const bool modifiersDown = std::ranges::all_of(keyAliases | std::views::take(keyAliases.size() - 1),
                                                   [this](const std::string &alias) { return IsKeyDown(alias); });
    return modifiersDown && IsKeyPressed(keyAliases.back());
}


//
// Mouse
//

bool Core::InputManager::IsValidMouseButton(const int button) {
    return button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST;
}

void Core::InputManager::HandleClickEvent(const int button, const int action, int mods) {
    if (!IsValidMouseButton(button))
        return;
    mouseButtons[button] = action != GLFW_RELEASE;
}

bool Core::InputManager::IsMouseDown(const int button) const {
    return IsValidMouseButton(button) && mouseButtons[button];
}

bool Core::InputManager::IsMouseDown(const std::string &buttonAlias) const {
    return IsMouseDown(GetKeyFromName(buttonAlias));
}

bool Core::InputManager::IsMousePressed(const int button) const {
    return IsValidMouseButton(button) && mouseButtons[button] && !previousMouseButtons[button];
}

bool Core::InputManager::IsMousePressed(const std::string &buttonAlias) const {
    return IsMousePressed(GetKeyFromName(buttonAlias));
}

bool Core::InputManager::IsMouseReleased(const int button) const {
    return IsValidMouseButton(button) && !mouseButtons[button] && previousMouseButtons[button];
}

bool Core::InputManager::IsMouseReleased(const std::string &buttonAlias) const {
    return IsMouseReleased(GetKeyFromName(buttonAlias));
}

void Core::InputManager::SetMousePos(const double xPos, const double yPos) {
    if (m_FirstMouse) {
        m_MousePosX = xPos;
        m_MousePosY = yPos;
        m_FirstMouse = false;
    }

    m_MouseDeltaX += xPos - m_MousePosX;
    m_MouseDeltaY += yPos - m_MousePosY;

    m_MousePosX = xPos;
    m_MousePosY = yPos;
}


void Core::InputManager::HandleScrollEvent(const double xOffset, const double yOffset) {
    m_ScrollX += xOffset;
    m_ScrollY += yOffset;
}

// key mappings (GetKeyFromName) are in KeyMappings.cpp
