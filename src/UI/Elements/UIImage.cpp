#include "UIImage.h"
#include "UI/Rendering/UIRenderer.h"

namespace UI {

    void UIImage::Update() {
        //TODO
    }

    void UIImage::Draw(UIRenderer *renderer, const glm::vec2 finalPos) {
        if (!renderer)
            return;

        if (Rect.Scale.x <= 0.0f || Rect.Scale.y <= 0.0f)
            return;

        if (m_Image) {
            renderer->SubmitQuad(finalPos, Rect.Scale, {0.0f, 0.0f}, {1.0f, 1.0f}, m_Image.get(), m_Color);
        } else {
            renderer->SubmitRect(finalPos, Rect.Scale, m_Color);
        }
    }

} // namespace UI
