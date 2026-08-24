#pragma once

#include "UI/Rendering/UIRenderer.h"
#include "UI/UIElement.h"
#include "UI/Elements/UIButton.h"
#include "Platform/Input/InputManager.h"
#include <glm/vec2.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace UI {

    [[nodiscard]] inline bool IsPointInsideRect(const glm::vec2 &point, const RectTransform &rect) {
        return point.x >= rect.Position.x && point.x < rect.Position.x + rect.Scale.x && point.y >= rect.Position.y && point.y < rect.Position.y + rect.Scale.y;
    }

    class UISystem {
    public:
        UISystem(UIRenderer *renderer, Platform::Input::InputManager *input) : m_Renderer(renderer), m_Input(input) {
            m_Root = std::make_unique<UIElement>();
            m_Root->Name = "Canvas";
            m_Root->SetInputMgr(m_Input);
            m_NameIndex[m_Root->Name] = m_Root.get();
        }

        void SetRoot(std::unique_ptr<UIElement> el) {
            m_Root = std::move(el);
            SetInputMgrRecursive(m_Root.get(), m_Input);
            RebuildNameIndex();
        }
        [[nodiscard]] UIElement *GetRoot() { return m_Root.get(); }
        [[nodiscard]] const UIElement *GetRoot() const { return m_Root.get(); }

        UIElement *AddChild(UIElement *parent, std::unique_ptr<UIElement> element);

        void RemoveChild(UIElement *parent, UIElement *child);

        void Clear();

        void Update(float dt) const;
        void Render();
        [[nodiscard]] UIElement *HitTest(glm::vec2 point) const;

        [[nodiscard]] UIElement *FindByName(const std::string &name);
        [[nodiscard]] const UIElement *FindByName(const std::string &name) const;

        void OnElementRenamed(UIElement *element, const std::string &oldName);

        void SnapshotButtonStates();
        [[nodiscard]] ButtonState GetButtonState(const std::string &name) const;

    private:
        void RenderRecursive(UIElement *element, glm::vec2 accumulatedPos);
        static UIElement *HitTestRecursive(UIElement *element, glm::vec2 point, glm::vec2 accumulatedPos);

        void RebuildNameIndex();
        void IndexSubtreeRecursive(UIElement *element);
        static void SetInputMgrRecursive(UIElement *element, Platform::Input::InputManager *mgr) {
            if (!element)
                return;
            element->SetInputMgr(mgr);
            for (auto *child : element->Children) {
                SetInputMgrRecursive(child, mgr);
            }
        }

        std::unique_ptr<UIElement> m_Root;
        std::vector<std::unique_ptr<UIElement>> m_OwnedElements;
        UIRenderer *m_Renderer = nullptr;
        Platform::Input::InputManager *m_Input = nullptr;
        std::unordered_map<std::string, ButtonState> m_ButtonSnapshots;

        // name -> element
        std::unordered_map<std::string, UIElement *> m_NameIndex;
    };

} // namespace UI
