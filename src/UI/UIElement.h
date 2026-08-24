#pragma once
#include "RectTransform.h"
#include "Platform/Input/InputManager.h"

#include <string>
#include <vector>

namespace UI {

    enum UIFlags : uint8_t { VISIBLE = 1 << 0, ENABLED = 1 << 1, HOVERED = 1 << 2, FOCUSED = 1 << 3 };

    class UIRenderer;

    class UIElement {
    public:
        virtual ~UIElement() = default;
        std::string Name;
        UIElement *Parent = nullptr;
        std::vector<UIElement *> Children;
        RectTransform Rect{.Position = {0.0f, 0.0f}, .Scale = {0.0f, 0.0f}};

        virtual void Update() {}

        virtual void Draw(UIRenderer * /*renderer*/, glm::vec2 /*finalPos*/) {}

        [[nodiscard]] bool HasFlag(const UIFlags flag) const { return Flags & flag; }
        void AddFlag(const UIFlags flag) { Flags |= flag; }
        void RemoveFlag(const UIFlags flag) { Flags &= ~flag; }

        void SetInputMgr(Platform::Input::InputManager *mgr) { m_Input = mgr; }

        void SetGameMousePos(const glm::vec2 &pos) { m_GameMousePos = pos; }
        [[nodiscard]] const glm::vec2 &GetGameMousePos() const { return m_GameMousePos; }

    protected:
        Platform::Input::InputManager *m_Input = nullptr; // set by system
        glm::vec2 m_GameMousePos = {0.0f, 0.0f};
        uint8_t Flags = VISIBLE | ENABLED;
    };

} // namespace UI
