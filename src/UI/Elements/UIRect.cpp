#include "UIRect.h"
#include "UI/Rendering/UIRenderer.h"

namespace UI {

    void UIRect::Draw(UIRenderer *renderer, const glm::vec2 finalPos) {
        if (!renderer)
            return;
        if (Rect.Scale.x > 0.0f && Rect.Scale.y > 0.0f)
            renderer->SubmitRect(finalPos, Rect.Scale, m_Color);
    }

} // namespace UI
