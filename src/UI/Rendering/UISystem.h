#pragma once

#include "UI/UIElement.h"
#include "UI/Rendering/UIRenderer.h"
#include "Platform/Input/InputManager.h"
#include <glm/vec2.hpp>
#include <memory>

namespace UI {

    [[nodiscard]] inline bool IsPointInsideRect(const glm::vec2 &point, const RectTransform &rect) {
        return point.x >= rect.Position.x && point.x < rect.Position.x + rect.Scale.x &&
               point.y >= rect.Position.y && point.y < rect.Position.y + rect.Scale.y;
    }

    class UISystem {
    public:
        UISystem(UIRenderer *renderer, Platform::Input::InputManager *input)
            : m_Renderer(renderer), m_Input(input) {
            m_Root = std::make_unique<UIElement>();
            m_Root->Name = "Canvas";
        }

        // Replace the root element (UISystem takes ownership)
        void SetRoot(std::unique_ptr<UIElement> el) { m_Root = std::move(el); }
        [[nodiscard]] UIElement *GetRoot() { return m_Root.get(); }
        [[nodiscard]] const UIElement *GetRoot() const { return m_Root.get(); }

        void Update(float dt);
        void Render();
        [[nodiscard]] UIElement *HitTest(glm::vec2 point) const;

    private:
        void RenderRecursive(UIElement *element, glm::vec2 accumulatedPos);
        static UIElement *HitTestRecursive(UIElement *element, glm::vec2 point, glm::vec2 accumulatedPos);

        std::unique_ptr<UIElement> m_Root;
        UIRenderer *m_Renderer;
        Platform::Input::InputManager *m_Input;
    };

} // namespace UI
