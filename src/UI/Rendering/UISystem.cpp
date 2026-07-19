#include "UISystem.h"

#include <ranges>

namespace UI {

    static void UpdateRecursive(UIElement *element, const float dt) {
        if (!element || !element->HasFlag(VISIBLE))
            return;

        element->Update();

        for (auto *child : element->Children) {
            UpdateRecursive(child, dt);
        }
    }

    void UISystem::Update(const float dt) { UpdateRecursive(m_Root.get(), dt); }

    void UISystem::Render() { RenderRecursive(m_Root.get(), {0.0f, 0.0f}); }

    void UISystem::RenderRecursive(UIElement *element, const glm::vec2 accumulatedPos) {
        if (!element || !element->HasFlag(VISIBLE))
            return;

        const glm::vec2 finalPos = accumulatedPos + element->Rect.Position;

        // draw this element
        element->Draw(m_Renderer, finalPos);

        // draw children
        for (auto *child : element->Children) {
            RenderRecursive(child, finalPos);
        }
    }

    UIElement *UISystem::HitTest(const glm::vec2 point) { return HitTestRecursive(m_Root.get(), point, {0.0f, 0.0f}); }

    UIElement *UISystem::HitTestRecursive(UIElement *element, const glm::vec2 point, const glm::vec2 accumulatedPos) {
        if (!element || !element->HasFlag(VISIBLE) || !element->HasFlag(ENABLED))
            return nullptr;

        const glm::vec2 finalPos = accumulatedPos + element->Rect.Position;

        // check children in reverse
        for (const auto &it : std::views::reverse(element->Children)) {
            if (auto *hit = HitTestRecursive(it, point, finalPos))
                return hit;
        }

        // check self
        if (IsPointInsideRect(point, {finalPos, element->Rect.Scale}))
            return element;

        return nullptr;
    }

} // namespace UI
