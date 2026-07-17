#pragma once
#include "UI/UIElement.h"
#include "UI/Text/Font.h"
#include <glm/vec4.hpp>
#include <memory>
#include <string>
#include <vector>

namespace UI {

    struct TextVertex {
        glm::vec2 Position;
        glm::vec2 UV;
    };

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

        // quad vertices for each char
        [[nodiscard]] std::vector<TextVertex> BuildVertices() const;

        // (skips glyphs with zero size , e.g. spaces)
        [[nodiscard]] size_t GetQuadCount() const;

        void Update() override;
        void Draw() override;

    private:
        std::string m_Text;
        std::shared_ptr<Font> m_Font;
        glm::vec4 m_Color = {1.0f, 1.0f, 1.0f, 1.0f};
    };

} // namespace UI
