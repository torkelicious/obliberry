#include <unordered_map>
#include "InputManager.h"

int Platform::Input::InputManager::GetKeyFromName(const std::string &keyName) {
    static const std::unordered_map<std::string, int> keyMap = {
            // Letters
            {"A", GLFW_KEY_A},
            {"B", GLFW_KEY_B},
            {"C", GLFW_KEY_C},
            {"D", GLFW_KEY_D},
            {"E", GLFW_KEY_E},
            {"F", GLFW_KEY_F},
            {"G", GLFW_KEY_G},
            {"H", GLFW_KEY_H},
            {"I", GLFW_KEY_I},
            {"J", GLFW_KEY_J},
            {"K", GLFW_KEY_K},
            {"L", GLFW_KEY_L},
            {"M", GLFW_KEY_M},
            {"N", GLFW_KEY_N},
            {"O", GLFW_KEY_O},
            {"P", GLFW_KEY_P},
            {"Q", GLFW_KEY_Q},
            {"R", GLFW_KEY_R},
            {"S", GLFW_KEY_S},
            {"T", GLFW_KEY_T},
            {"U", GLFW_KEY_U},
            {"V", GLFW_KEY_V},
            {"W", GLFW_KEY_W},
            {"X", GLFW_KEY_X},
            {"Y", GLFW_KEY_Y},
            {"Z", GLFW_KEY_Z},

            // Top Row nums
            {"0", GLFW_KEY_0},
            {"1", GLFW_KEY_1},
            {"2", GLFW_KEY_2},
            {"3", GLFW_KEY_3},
            {"4", GLFW_KEY_4},
            {"5", GLFW_KEY_5},
            {"6", GLFW_KEY_6},
            {"7", GLFW_KEY_7},
            {"8", GLFW_KEY_8},
            {"9", GLFW_KEY_9},

            // Navigation / Special
            {"Space", GLFW_KEY_SPACE},
            {"Esc", GLFW_KEY_ESCAPE},
            {"Escape", GLFW_KEY_ESCAPE}, // Alias
            {"Enter", GLFW_KEY_ENTER},
            {"Tab", GLFW_KEY_TAB},
            {"Backspace", GLFW_KEY_BACKSPACE},
            {"Insert", GLFW_KEY_INSERT},
            {"Delete", GLFW_KEY_DELETE},
            {"Right", GLFW_KEY_RIGHT},
            {"Left", GLFW_KEY_LEFT},
            {"Down", GLFW_KEY_DOWN},
            {"Up", GLFW_KEY_UP},
            {"PageUp", GLFW_KEY_PAGE_UP},
            {"PageDown", GLFW_KEY_PAGE_DOWN},
            {"Home", GLFW_KEY_HOME},
            {"End", GLFW_KEY_END},
            {"CapsLock", GLFW_KEY_CAPS_LOCK},
            {"ScrollLock", GLFW_KEY_SCROLL_LOCK},
            {"NumLock", GLFW_KEY_NUM_LOCK},
            {"PrintScreen", GLFW_KEY_PRINT_SCREEN},
            {"Pause", GLFW_KEY_PAUSE},

            // Modifiers
            {"LeftShift", GLFW_KEY_LEFT_SHIFT},
            {"RightShift", GLFW_KEY_RIGHT_SHIFT},
            {"Shift", GLFW_KEY_LEFT_SHIFT}, // Alias defaults to Left
            {"LeftCtrl", GLFW_KEY_LEFT_CONTROL},
            {"RightCtrl", GLFW_KEY_RIGHT_CONTROL},
            {"Ctrl", GLFW_KEY_LEFT_CONTROL}, // Alias defaults to Left
            {"LeftAlt", GLFW_KEY_LEFT_ALT},
            {"RightAlt", GLFW_KEY_RIGHT_ALT},
            {"Alt", GLFW_KEY_LEFT_ALT}, // Alias defaults to Left
            {"LeftSuper", GLFW_KEY_LEFT_SUPER},
            {"RightSuper", GLFW_KEY_RIGHT_SUPER},
            {"Menu", GLFW_KEY_MENU},

            // Function Keys
            {"F1", GLFW_KEY_F1},
            {"F2", GLFW_KEY_F2},
            {"F3", GLFW_KEY_F3},
            {"F4", GLFW_KEY_F4},
            {"F5", GLFW_KEY_F5},
            {"F6", GLFW_KEY_F6},
            {"F7", GLFW_KEY_F7},
            {"F8", GLFW_KEY_F8},
            {"F9", GLFW_KEY_F9},
            {"F10", GLFW_KEY_F10},
            {"F11", GLFW_KEY_F11},
            {"F12", GLFW_KEY_F12},

            // Numpad
            {"Num0", GLFW_KEY_KP_0},
            {"Num1", GLFW_KEY_KP_1},
            {"Num2", GLFW_KEY_KP_2},
            {"Num3", GLFW_KEY_KP_3},
            {"Num4", GLFW_KEY_KP_4},
            {"Num5", GLFW_KEY_KP_5},
            {"Num6", GLFW_KEY_KP_6},
            {"Num7", GLFW_KEY_KP_7},
            {"Num8", GLFW_KEY_KP_8},
            {"Num9", GLFW_KEY_KP_9},
            {"NumDecimal", GLFW_KEY_KP_DECIMAL},
            {"NumDivide", GLFW_KEY_KP_DIVIDE},
            {"NumMultiply", GLFW_KEY_KP_MULTIPLY},
            {"NumSubtract", GLFW_KEY_KP_SUBTRACT},
            {"NumAdd", GLFW_KEY_KP_ADD},
            {"NumEnter", GLFW_KEY_KP_ENTER},
            {"NumEqual", GLFW_KEY_KP_EQUAL},

            // Symbols / Punctuation
            {"Apostrophe", GLFW_KEY_APOSTROPHE},
            {"Comma", GLFW_KEY_COMMA},
            {"Minus", GLFW_KEY_MINUS},
            {"Period", GLFW_KEY_PERIOD},
            {"Slash", GLFW_KEY_SLASH},
            {"Semicolon", GLFW_KEY_SEMICOLON},
            {"Equal", GLFW_KEY_EQUAL},
            {"LeftBracket", GLFW_KEY_LEFT_BRACKET},
            {"RightBracket", GLFW_KEY_RIGHT_BRACKET},
            {"Backslash", GLFW_KEY_BACKSLASH},
            {"GraveAccent", GLFW_KEY_GRAVE_ACCENT},

            // Mouse Buttons
            {"MouseLeft", GLFW_MOUSE_BUTTON_LEFT},
            {"MouseRight", GLFW_MOUSE_BUTTON_RIGHT},
            {"MouseMiddle", GLFW_MOUSE_BUTTON_MIDDLE},
            {"MouseButton1", GLFW_MOUSE_BUTTON_1},
            {"MouseButton2", GLFW_MOUSE_BUTTON_2},
            {"MouseButton3", GLFW_MOUSE_BUTTON_3},
            {"MouseButton4", GLFW_MOUSE_BUTTON_4}, // often'Back' button
            {"MouseButton5", GLFW_MOUSE_BUTTON_5}, // often 'Forward' button
            {"MouseButton6", GLFW_MOUSE_BUTTON_6},
            {"MouseButton7", GLFW_MOUSE_BUTTON_7},
            {"MouseButton8", GLFW_MOUSE_BUTTON_8}};

    if (const auto it = keyMap.find(keyName); it != keyMap.end()) {
        return it->second;
    }

    return -1; // invalid key
}
