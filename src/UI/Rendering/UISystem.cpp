#include "UISystem.h"

#include <algorithm>
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
        if (!element || !element->HasFlag(VISIBLE) || !element->HasFlag(ENABLED))
            return;

        const glm::vec2 finalPos = accumulatedPos + element->Rect.Position;

        // draw this element
        element->Draw(m_Renderer, finalPos);

        // draw children
        for (auto *child : element->Children) {
            RenderRecursive(child, finalPos);
        }
    }

    UIElement *UISystem::HitTest(const glm::vec2 point) const { return HitTestRecursive(m_Root.get(), point, {0.0f, 0.0f}); }

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

    UIElement *UISystem::AddChild(UIElement *parent, std::unique_ptr<UIElement> element) {
        if (!parent || !element)
            return nullptr;
        UIElement *raw = element.get();
        raw->Parent = parent;
        parent->Children.push_back(raw);
        m_OwnedElements.push_back(std::move(element));
        return raw;
    }

    void UISystem::RemoveChild(UIElement *parent, UIElement *child) {
        if (!parent || !child)
            return;

        // copy children list before modifying
        auto childCopy = child->Children;
        for (auto *grandchild : childCopy) {
            RemoveChild(child, grandchild);
        }

        // Remove from parent
        auto &children = parent->Children;
        std::erase(children, child);
        child->Parent = nullptr;

        // Remove from owned elements
        std::erase_if(m_OwnedElements, [child](const std::unique_ptr<UIElement> &ptr) { return ptr.get() == child; });
    }

} // namespace UI
