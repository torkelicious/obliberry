#include "UIButton.h"
#include "UI/Rendering/UIRenderer.h"
#include "Rendering/Texture.h"

namespace UI {

    void UIButton::Update() {
        // TODO
    }

    void UIButton::Draw(UIRenderer *renderer, const glm::vec2 finalPos) {
        if (!renderer)
            return;

        // background
        if (Rect.Scale.x > 0.0f && Rect.Scale.y > 0.0f) {
            renderer->SubmitRect(finalPos, Rect.Scale, m_BackgroundColor);
        }

        // text
        if (!m_Text.empty() && m_Font) {
            const auto atlas = m_Font->GetAtlasTexture();
            if (!atlas)
                return;

            const bool isSDF = m_Font->IsSDF();
            const unsigned int spread = m_Font->GetSDFSpread();

            // center text
            const float textWidth = GetTextWidth();
            const float textX = finalPos.x + (Rect.Scale.x - textWidth) * 0.5f;
            const float textY = finalPos.y + (Rect.Scale.y + static_cast<float>(m_Font->GetFontSize())) * 0.5f;

            float cursorX = 0.0f;
            for (const char c : m_Text) {
                const auto &glyph = m_Font->GetGlyph(c);
                const auto &Size = glyph.Size;
                const auto &Bearing = glyph.Bearing;
                const auto &Advance = glyph.Advance;
                const auto &UVOffset = glyph.UVOffset;
                const auto &UVSize = glyph.UVSize;

                if (Size.x > 0 && Size.y > 0) {
                    const float x = textX + cursorX + static_cast<float>(Bearing.x);
                    const float y = textY - static_cast<float>(Bearing.y);
                    const auto w = static_cast<float>(Size.x);
                    const auto h = static_cast<float>(Size.y);

                    const glm::vec2 uvMin = UVOffset;
                    const glm::vec2 uvMax = UVOffset + UVSize;

                    if (isSDF) {
                        renderer->SubmitSDFQuad({x, y}, {w, h}, uvMin, uvMax, atlas.get(), m_Color, 1.0f, static_cast<float>(spread));
                    } else {
                        renderer->SubmitQuad({x, y}, {w, h}, uvMin, uvMax, atlas.get(), m_Color);
                    }
                }

                cursorX += static_cast<float>(Advance);
            }
        }
    }

} // namespace UI
