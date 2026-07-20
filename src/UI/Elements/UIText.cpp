#include "UIText.h"
#include "UI/Rendering/UIRenderer.h"

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

        float cursorX = 0.0f;

        for (const char c : m_Text) {
            const auto &glyph = m_Font->GetGlyph(c);

            if (glyph.Size.x > 0 && glyph.Size.y > 0) {
                const float x = finalPos.x + cursorX + static_cast<float>(glyph.Bearing.x);
                const float y = finalPos.y - static_cast<float>(glyph.Bearing.y);
                const auto w = static_cast<float>(glyph.Size.x);
                const auto h = static_cast<float>(glyph.Size.y);

                const glm::vec2 uvMin = glyph.UVOffset;
                const glm::vec2 uvMax = glyph.UVOffset + glyph.UVSize;

                renderer->SubmitQuad({x, y}, {w, h}, uvMin, uvMax, atlas.get(), m_Color);
            }

            cursorX += static_cast<float>(glyph.Advance);
        }
    }

    std::vector<TextVertex> UIText::BuildVertices() const {
        if (!m_Font || m_Text.empty())
            return {};

        std::vector<TextVertex> vertices;
        vertices.reserve(m_Text.size() * 6); // two triangles per char

        float cursorX = 0.0f;

        for (const char c : m_Text) {
            const auto &[Size, Bearing, Advance, UVOffset, UVSize] = m_Font->GetGlyph(c);
            if (Size.x > 0 && Size.y > 0) {
                const float baselineY = 0.0f;
                const float x = cursorX + static_cast<float>(Bearing.x);
                const float y = baselineY - static_cast<float>(Size.y - Bearing.y);
                const auto w = static_cast<float>(Size.x);
                const auto h = static_cast<float>(Size.y);

                const float u0 = UVOffset.x;
                const float v0 = UVOffset.y;
                const float u1 = UVOffset.x + UVSize.x;
                const float v1 = UVOffset.y + UVSize.y;

                // triangle 1
                vertices.push_back({{x, y}, {u0, v0}});
                vertices.push_back({{x + w, y}, {u1, v0}});
                vertices.push_back({{x + w, y + h}, {u1, v1}});
                // triangle 2
                vertices.push_back({{x, y}, {u0, v0}});
                vertices.push_back({{x + w, y + h}, {u1, v1}});
                vertices.push_back({{x, y + h}, {u0, v1}});
            }

            cursorX += static_cast<float>(Advance);
        }
        return vertices;
    }

} // namespace UI
