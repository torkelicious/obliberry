#pragma once
#include "UI/UIElement.h"

namespace UI {

    class UIRect : public UIElement {
    public:
        void SetColor(const glm::vec4 color) { m_Color = color; }
        [[nodiscard]] const glm::vec4 &GetColor() const { return m_Color; }

        void Draw(UIRenderer *renderer, glm::vec2 finalPos) override;

    private:
        glm::vec4 m_Color = {0.3f, 0.3f, 0.3f, 1.0f};
    };

} // namespace UI
