#pragma once
#include "UI/UIElement.h"

namespace UI {
    enum class ButtonState : uint8_t {
        NONE,
        CLICKED,
        HELD

    };
    class UIButton : public UIElement {
    public:
        void Update() override;
        void Draw() override;
    private:
        ButtonState ButtonState = ButtonState::NONE;
    };
} // namespace UI
