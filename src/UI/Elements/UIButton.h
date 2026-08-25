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
        void SetText(const std::string &text) {
            if (m_Text != text) {
                m_Text = text;
                m_LayoutDirty = true;
            }
        }
        void SetFont(std::shared_ptr<Font> font) {
            if (m_Font != font) {
                m_Font = std::move(font);
                m_LayoutDirty = true;
            }
        }
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
        ButtonState m_ButtonState = ButtonState::NONE;
        glm::vec4 m_Color = {1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 m_BackgroundColor = {0.2f, 0.2f, 0.2f, 1.0f};
        glm::vec4 m_HoveredBackgroundColor = {0.1f, 0.1f, 0.1f, 1.0f};
        std::shared_ptr<Rendering::Texture> m_BackgroundTexture;

        std::string m_Text;
        std::shared_ptr<Font> m_Font;

        // cached text metrics
        mutable bool m_LayoutDirty = true;
        mutable float m_NaturalWidth = 0.0f;
        mutable float m_MaxHeight = 0.0f;
        mutable float m_MaxBearingY = 0.0f;
    };
} // namespace UI
