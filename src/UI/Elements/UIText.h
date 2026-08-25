#pragma once
#include "UI/UIElement.h"
#include "UI/Text/Font.h"
#include <glm/vec4.hpp>
#include <memory>
#include <string>
#include <vector>

namespace UI {

    class UIText : public UIElement {
    public:
        void SetText(const std::string &text);
        void SetFont(std::shared_ptr<Font> font);
        void SetColor(glm::vec4 color);

        [[nodiscard]] const std::string &GetText() const { return m_Text; }
        [[nodiscard]] const std::shared_ptr<Font> &GetFont() const { return m_Font; }
        [[nodiscard]] const glm::vec4 &GetColor() const { return m_Color; }

        // the text width in pixels at the current font size
        [[nodiscard]] float GetTextWidth() const;

        // (skips glyphs with zero size , e.g. spaces)
        [[nodiscard]] size_t GetQuadCount() const;

        void Update() override;
        void Draw(UIRenderer *renderer, glm::vec2 finalPos) override;

    private:
        std::string m_Text;
        std::shared_ptr<Font> m_Font;
        glm::vec4 m_Color = {1.0f, 1.0f, 1.0f, 1.0f};

        // cached text metric
        mutable bool m_LayoutDirty = true;
        mutable float m_NaturalWidth = 0.0f;
        mutable float m_MaxHeight = 0.0f;
        mutable float m_MaxBearingY = 0.0f;
    };

} // namespace UI
