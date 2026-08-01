#include <glad/glad.h>
#include "UIText.h"
#include "UI/Rendering/UIRenderer.h"
#include "Rendering/Texture.h"
#include <algorithm>

namespace UI {


    void UIText::SetText(const std::string &text) { m_Text = text; }

    void UIText::SetFont(std::shared_ptr<Font> font) { m_Font = std::move(font); }

    void UIText::SetColor(const glm::vec4 color) { m_Color = color; }

    float UIText::GetTextWidth() const {
        if (!m_Font || m_Text.empty())
            return 0.0f;

        float width = 0.0f;
        for (const char c : m_Text) {
            const auto &glyph = m_Font->GetGlyph(c);
            width += static_cast<float>(glyph.Advance);
        }
        return width;
    }

    size_t UIText::GetQuadCount() const {
        if (!m_Font || m_Text.empty())
            return 0;

        size_t count = 0;
        for (const char c : m_Text) {
            if (const auto &glyph = m_Font->GetGlyph(c); glyph.Size.x > 0 && glyph.Size.y > 0) {
                count++;
            }
        }
        return count;
    }

    //
    // stuff here :)
    //

    void UIText::Update() {}

    void UIText::Draw(UIRenderer *renderer, const glm::vec2 finalPos) {
        if (!renderer || !m_Font || m_Text.empty())
            return;

        const auto atlas = m_Font->GetAtlasTexture();
        if (!atlas)
            return;

        float naturalWidth = 0.0f;
        float maxHeight = 0.0f;
        float maxBearingY = 0.0f;
        for (const char c : m_Text) {
            const auto &glyph = m_Font->GetGlyph(c);
            naturalWidth += static_cast<float>(glyph.Advance);
            maxHeight = std::max(maxHeight, static_cast<float>(glyph.LayoutSize.y));
            maxBearingY = std::max(maxBearingY, static_cast<float>(glyph.Bearing.y));
        }

        float scale = 1.0f;
        if (naturalWidth > 0.0f && maxHeight > 0.0f && Rect.Scale.x > 0.0f && Rect.Scale.y > 0.0f) {
            const float sx = Rect.Scale.x / naturalWidth;
            const float sy = Rect.Scale.y / maxHeight;
            scale = std::min(sx, sy);
        }

        const bool isSDF = m_Font->IsSDF();
        const unsigned int spread = m_Font->GetSDFSpread();
        const float sdfRenderScale = isSDF ? scale : 1.0f;

        const bool centerVertically = Rect.Scale.y > 0.0f;
        const float textTop = finalPos.y + (centerVertically ? (Rect.Scale.y - maxHeight * scale) * 0.5f : 0.0f);
        const float baselineAnchor = textTop + (centerVertically ? maxBearingY * scale : 0.0f);

        float cursorX = 0.0f;

        for (const char c : m_Text) {
            const auto &glyph = m_Font->GetGlyph(c);

            if (glyph.Size.x > 0 && glyph.Size.y > 0) {
                const float x = finalPos.x + cursorX * scale + static_cast<float>(glyph.Bearing.x) * scale;
                const float y = baselineAnchor - static_cast<float>(glyph.Bearing.y) * scale;
                const auto w = static_cast<float>(glyph.Size.x) * scale;
                const auto h = static_cast<float>(glyph.Size.y) * scale;

                // prevent linear filter bleeding from adjacent atlas glyphs
                const auto atlasW = static_cast<float>(atlas->GetWidth());
                const auto atlasH = static_cast<float>(atlas->GetHeight());
                const glm::vec2 halfTexel = {0.5f / atlasW, 0.5f / atlasH};
                const glm::vec2 uvMin = glyph.UVOffset + halfTexel;
                const glm::vec2 uvMax = glyph.UVOffset + glyph.UVSize - halfTexel;

                if (isSDF) {
                    renderer->SubmitSDFQuad({x, y}, {w, h}, uvMin, uvMax, atlas.get(), m_Color, sdfRenderScale, static_cast<float>(spread));
                } else {
                    renderer->SubmitQuad({x, y}, {w, h}, uvMin, uvMax, atlas.get(), m_Color);
                }
            }

            cursorX += static_cast<float>(glyph.Advance);
        }
    }

} // namespace UI
