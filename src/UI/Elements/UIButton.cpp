#include "UIButton.h"
#include "UI/Rendering/UIRenderer.h"
#include "Rendering/Texture.h"
#include "UI/Rendering/UISystem.h"

namespace UI {

    void UIButton::Update() {
        if (IsPointInsideRect(m_GameMousePos, Rect)) {
            if (m_Input->IsMousePressed(0)) {
                m_ButtonState = ButtonState::CLICKED;
            } else if (m_Input->IsMouseDown(0)) {
                m_ButtonState = ButtonState::HELD;
            } else {
                m_ButtonState = ButtonState::HOVERED;
            }
        } else {
            m_ButtonState = ButtonState::NONE;
        }
    }

    void UIButton::Draw(UIRenderer *renderer, const glm::vec2 finalPos) {
        if (!renderer)
            return;

        // background
        if (Rect.Scale.x > 0.0f && Rect.Scale.y > 0.0f) {
            const glm::vec4 &bgCol = (m_ButtonState == ButtonState::HOVERED || m_ButtonState == ButtonState::HELD) ? m_HoveredBackgroundColor : m_BackgroundColor;
            if (m_BackgroundTexture) {
                renderer->SubmitQuad(finalPos, Rect.Scale, {0.0f, 0.0f}, {1.0f, 1.0f}, m_BackgroundTexture.get(), bgCol);
            } else {
                renderer->SubmitRect(finalPos, Rect.Scale, bgCol);
            }
        }

        // text
        if (!m_Text.empty() && m_Font) {
            const auto atlas = m_Font->GetAtlasTexture();
            if (!atlas)
                return;

            const bool isSDF = m_Font->IsSDF();
            const unsigned int spread = m_Font->GetSDFSpread();

            // center text vertically
            // ascent ~ 0.8 * fontSize, descent ~ 0.2 * fontSize so offset ~ 0.3 * fontSize
            // i think at least
            const float textWidth = GetTextWidth();
            const float textX = finalPos.x + (Rect.Scale.x - textWidth) * 0.5f;
            const float textY = finalPos.y + Rect.Scale.y * 0.5f + static_cast<float>(m_Font->GetFontSize()) * 0.3f;

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

                    // prevent linear filter bleeding from adjacent atlas glyphs
                    const auto atlasW = static_cast<float>(atlas->GetWidth());
                    const auto atlasH = static_cast<float>(atlas->GetHeight());
                    const glm::vec2 halfTexel = {0.5f / atlasW, 0.5f / atlasH};
                    const glm::vec2 uvMin = UVOffset + halfTexel;
                    const glm::vec2 uvMax = UVOffset + UVSize - halfTexel;

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
