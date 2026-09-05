#include "UIButton.h"
#include "UI/Rendering/UIRenderer.h"
#include "Rendering/Types/Texture/Texture.h"
#include "UI/Rendering/UISystem.h"
#include <algorithm>

namespace UI {

    void UIButton::Update() {
        if (!m_Input) {
            m_ButtonState = ButtonState::NONE;
            return;
        }

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
            const glm::vec4 &bgCol = m_ButtonState == ButtonState::HOVERED || m_ButtonState == ButtonState::HELD ? m_HoveredBackgroundColor : m_BackgroundColor;
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

            // text metrics only change when text or font changes
            if (m_LayoutDirty) {
                m_NaturalWidth = 0.0f;
                m_MaxHeight = 0.0f;
                m_MaxBearingY = 0.0f;
                for (const char c : m_Text) {
                    const auto &glyph = m_Font->GetGlyph(c);
                    m_NaturalWidth += static_cast<float>(glyph.Advance);
                    m_MaxHeight = std::max(m_MaxHeight, static_cast<float>(glyph.LayoutSize.y));
                    m_MaxBearingY = std::max(m_MaxBearingY, static_cast<float>(glyph.Bearing.y));
                }
                m_LayoutDirty = false;
            }
            const float naturalWidth = m_NaturalWidth;
            const float maxHeight = m_MaxHeight;
            const float maxBearingY = m_MaxBearingY;

            float scale = 1.0f;
            if (naturalWidth > 0.0f && maxHeight > 0.0f && Rect.Scale.x > 0.0f && Rect.Scale.y > 0.0f) {
                const float sx = Rect.Scale.x / naturalWidth;
                const float sy = Rect.Scale.y / maxHeight;
                scale = std::min(sx, sy);
            }

            const float textX = finalPos.x + (Rect.Scale.x - naturalWidth * scale) * 0.5f;
            const float textTop = finalPos.y + (Rect.Scale.y - maxHeight * scale) * 0.5f;
            const float textY = textTop + maxBearingY * scale;

            const bool isSDF = m_Font->IsSDF();
            const unsigned int spread = m_Font->GetSDFSpread();
            const float sdfRenderScale = isSDF ? scale : 1.0f;
            const glm::vec2 halfTexel = {0.5f / static_cast<float>(atlas->GetWidth()), 0.5f / static_cast<float>(atlas->GetHeight())};

            float cursorX = 0.0f;
            for (const char c : m_Text) {
                const auto &glyph = m_Font->GetGlyph(c);
                const auto &Size = glyph.Size;
                const auto &Bearing = glyph.Bearing;
                const auto &Advance = glyph.Advance;
                const auto &UVOffset = glyph.UVOffset;
                const auto &UVSize = glyph.UVSize;

                if (Size.x > 0 && Size.y > 0) {
                    const float padX = (static_cast<float>(Size.x) - static_cast<float>(glyph.LayoutSize.x)) * 0.5f;
                    const float padY = (static_cast<float>(Size.y) - static_cast<float>(glyph.LayoutSize.y)) * 0.5f;
                    const float x = textX + cursorX * scale + (static_cast<float>(Bearing.x) - padX) * scale;
                    const float y = textY - (static_cast<float>(Bearing.y) + padY) * scale;
                    const auto w = static_cast<float>(Size.x) * scale;
                    const auto h = static_cast<float>(Size.y) * scale;

                    // prevent linear filter bleeding from adjacent atlas glyphs
                    const glm::vec2 uvMin = UVOffset + halfTexel;
                    const glm::vec2 uvMax = UVOffset + UVSize - halfTexel;

                    if (isSDF) {
                        renderer->SubmitSDFQuad({x, y}, {w, h}, uvMin, uvMax, atlas.get(), m_Color, sdfRenderScale, static_cast<float>(spread));
                    } else {
                        renderer->SubmitQuad({x, y}, {w, h}, uvMin, uvMax, atlas.get(), m_Color);
                    }
                }

                cursorX += static_cast<float>(Advance);
            }
        }
    }

} // namespace UI
