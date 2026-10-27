#include "UISystem.h"
#include <functional>
#include <iterator>
#include <ranges>

namespace UI {

    static void UpdateRecursive(UIElement *element, const float dt, const glm::vec2 &gameMousePos, const bool interactive, const glm::vec2 accumulatedPos) {
        if (!element || !element->HasFlag(VISIBLE) || !element->HasFlag(ENABLED))
            return;

        const glm::vec2 finalPos = accumulatedPos + element->Rect.Position;
        element->SetGameMousePos(interactive ? gameMousePos : glm::vec2(-1e6f));
        element->Update(finalPos);

        for (auto *child : element->Children) {
            UpdateRecursive(child, dt, gameMousePos, interactive, finalPos);
        }
    }

    void UISystem::Update(const float dt, const bool interactive) const {
        glm::vec2 gameMouse = {0.0f, 0.0f};
        if (m_Renderer && m_Input) {
            gameMouse = m_Renderer->WindowToGameCoords(static_cast<float>(m_Input->MousePosX()), static_cast<float>(m_Input->MousePosY()));
        }
        UpdateRecursive(m_Root.get(), dt, gameMouse, interactive, {0.0f, 0.0f});
    }

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
        if (IsPointInsideRect(point, {.Position = finalPos, .Scale = element->Rect.Scale}))
            return element;

        return nullptr;
    }

    UIElement *UISystem::AddChild(UIElement *parent, std::unique_ptr<UIElement> element, Core::UUID uuid, const size_t insertIndex) {
        if (!Contains(parent) || !element) {
            return nullptr;
        }

        if (!uuid.IsNil()) {
            element->m_UUID = std::move(uuid);
        }

        if (m_UUIDIndex.contains(element->GetUUID())) {
            return nullptr;
        }

        UIElement *raw = element.get();
        raw->Parent = parent;
        raw->SetInputMgr(m_Input);
        if (insertIndex < parent->Children.size()) {
            parent->Children.insert(parent->Children.begin() + static_cast<std::ptrdiff_t>(insertIndex), raw);
        } else {
            parent->Children.push_back(raw);
        }
        IndexSubtreeRecursive(raw);
        m_OwnedElements.push_back(std::move(element));
        return raw;
    }

    void UISystem::RemoveChild(UIElement *parent, UIElement *child) {
        if (!parent || !child)
            return;

        // copy children list before modifying
        for (const auto childCopy = child->Children; auto *grandchild : childCopy) {
            RemoveChild(child, grandchild);
        }

        if (!child->Name.empty()) {
            if (const auto it = m_NameIndex.find(child->Name); it != m_NameIndex.end() && it->second == child) {
                m_NameIndex.erase(it);
            }
        }

        // Remove from parent
        auto &children = parent->Children;
        std::erase(children, child);
        child->Parent = nullptr;

        // Remove
        m_UUIDIndex.erase(child->GetUUID());
        std::erase_if(m_OwnedElements, [child](const std::unique_ptr<UIElement> &ptr) { return ptr.get() == child; });
    }

    void UISystem::Reparent(UIElement *child, UIElement *newParent) {
        if (!child || child == m_Root.get())
            return;

        if (!newParent)
            newParent = m_Root.get();

        if (child == newParent || child->Parent == newParent)
            return;

        if (!Contains(child) || !Contains(newParent))
            return;

        // moving a node under one of its own descendants would create a cycle
        for (UIElement *ancestor = newParent; ancestor; ancestor = ancestor->Parent) {
            if (ancestor == child)
                return;
        }

        // keep ownership; RemoveChild would destroy the element
        std::unique_ptr<UIElement> owned;
        for (auto it = m_OwnedElements.begin(); it != m_OwnedElements.end(); ++it) {
            if (it->get() == child) {
                owned = std::move(*it);
                m_OwnedElements.erase(it);
                break;
            }
        }

        if (!owned)
            return;

        if (child->Parent)
            std::erase(child->Parent->Children, child);

        child->Parent = newParent;
        child->SetInputMgr(m_Input);
        newParent->Children.push_back(child);

        m_OwnedElements.push_back(std::move(owned));
    }

    bool UISystem::Contains(const UIElement *element) const {
        if (!element)
            return false;
        const std::function<bool(const UIElement *)> search = [&](const UIElement *e) {
            if (!e)
                return false;
            if (e == element)
                return true;
            for (const auto *child : e->Children) {
                if (search(child))
                    return true;
            }
            return false;
        };
        return search(m_Root.get());
    }

    void UISystem::Clear() {
        // remove all children from root
        if (m_Root) {
            for (auto *child : m_Root->Children) {
                child->Parent = nullptr;
            }
            m_Root->Children.clear();
        }
        m_OwnedElements.clear();
        m_ButtonSnapshots.clear();
        RebuildIndices();
    }

    void UISystem::IndexSubtreeRecursive(UIElement *element) {
        if (!element) {
            return;
        }

        m_UUIDIndex[element->GetUUID()] = element;

        if (!element->Name.empty()) {
            m_NameIndex[element->Name] = element;
        }

        for (auto *child : element->Children) {
            IndexSubtreeRecursive(child);
        }
    }

    void UISystem::RebuildIndices() {
        m_NameIndex.clear();
        m_UUIDIndex.clear();
        IndexSubtreeRecursive(m_Root.get());
    }

    void UISystem::OnElementRenamed(UIElement *element, const std::string &oldName) {
        if (!element)
            return;
        if (!oldName.empty()) {
            if (const auto it = m_NameIndex.find(oldName); it != m_NameIndex.end() && it->second == element) {
                m_NameIndex.erase(it);
            }
        }
        if (!element->Name.empty()) {
            m_NameIndex[element->Name] = element;
        }
    }

    // Scripting
    UIElement *UISystem::FindByName(const std::string &name) {
        const auto it = m_NameIndex.find(name);
        return it != m_NameIndex.end() ? it->second : nullptr;
    }

    const UIElement *UISystem::FindByName(const std::string &name) const {
        const auto it = m_NameIndex.find(name);
        return it != m_NameIndex.end() ? it->second : nullptr;
    }

    UIElement *UISystem::FindByUUID(const Core::UUID &uuid) {
        const auto it = m_UUIDIndex.find(uuid);
        return it != m_UUIDIndex.end() ? it->second : nullptr;
    }

    const UIElement *UISystem::FindByUUID(const Core::UUID &uuid) const {
        const auto it = m_UUIDIndex.find(uuid);
        return it != m_UUIDIndex.end() ? it->second : nullptr;
    }

    static void SnapshotButtonsRecursive(UIElement *element, std::unordered_map<std::string, ButtonState> &out) {
        if (!element)
            return;
        if (const auto *btn = dynamic_cast<UIButton *>(element)) {
            if (!element->Name.empty())
                out[element->Name] = btn->GetButtonState();
        }
        for (auto *child : element->Children) {
            SnapshotButtonsRecursive(child, out);
        }
    }

    void UISystem::SnapshotButtonStates() {
        m_ButtonSnapshots.clear();
        SnapshotButtonsRecursive(m_Root.get(), m_ButtonSnapshots);
    }

    ButtonState UISystem::GetButtonState(const std::string &name) const {
        const auto it = m_ButtonSnapshots.find(name);
        return it != m_ButtonSnapshots.end() ? it->second : ButtonState::NONE;
    }

} // namespace UI
