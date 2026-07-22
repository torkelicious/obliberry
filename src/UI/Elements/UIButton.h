#pragma once
#include "Rendering/Texture.h"
#include "UI/UIElement.h"
#include "UI/Text/Font.h"

#include <memory>

namespace UI {
    enum class ButtonState : uint8_t {
        NONE,
        HOVERED,
        CLICKED,
        HELD

    };
    class UIButton : public UIElement {
    public:
        void SetText(const std::string &text) { m_Text = text; }
        void SetFont(std::shared_ptr<Font> font) { m_Font = std::move(font); }
        void SetColor(const glm::vec4 color) { m_Color = color; }
        void SetBackgroundColor(const glm::vec4 color) { m_BackgroundColor = color; }
        void SetBackgroundTexture(const std::shared_ptr<Rendering::Texture> &tex) { m_BackgroundTexture = tex; }
        void SetHoveredBackgroundColor(const glm::vec4 color) { m_HoveredBackgroundColor = color; }

        [[nodiscard]] const std::string &GetText() const { return m_Text; }
        [[nodiscard]] const std::shared_ptr<Font> &GetFont() const { return m_Font; }
        [[nodiscard]] const glm::vec4 &GetColor() const { return m_Color; }
        [[nodiscard]] const glm::vec4 &GetBackgroundColor() const { return m_BackgroundColor; }
        [[nodiscard]] const std::shared_ptr<Rendering::Texture> &GetBackgroundTexture() const { return m_BackgroundTexture; }
        [[nodiscard]] const glm::vec4 &GetHoveredBackgroundColor() const { return m_HoveredBackgroundColor; }
        [[nodiscard]] ButtonState GetButtonState() const { return m_ButtonState; }

        void Update() override;
        void Draw(UIRenderer *renderer, glm::vec2 finalPos) override;

    private:
        [[nodiscard]] float GetTextWidth() const {
            if (!m_Font || m_Text.empty())
                return 0.0f;
            float width = 0.0f;
            for (const char c : m_Text)
                width += static_cast<float>(m_Font->GetGlyph(c).Advance);
            return width;
        }

        ButtonState m_ButtonState = ButtonState::NONE;
        glm::vec4 m_Color = {1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 m_BackgroundColor = {0.2f, 0.2f, 0.2f, 1.0f};
        glm::vec4 m_HoveredBackgroundColor = {0.1f, 0.1f, 0.1f, 1.0f};
        std::shared_ptr<Rendering::Texture> m_BackgroundTexture;

        std::string m_Text;
        std::shared_ptr<Font> m_Font;
    };
} // namespace UI
