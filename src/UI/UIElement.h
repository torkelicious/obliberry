#pragma once
#include "RectTransform.h"
#include <string>
#include <vector>

namespace UI {

    enum UIFlags : uint8_t { VISIBLE = 1 << 0, ENABLED = 1 << 1, HOVERED = 1 << 2, FOCUSED = 1 << 3 };


    class UIElement {
    public:
        virtual ~UIElement() = default;
        std::string Name;
        UIElement *Parent;
        std::vector<UIElement *> Children;
        RectTransform Rect;
        virtual void Update();
        virtual void Draw();

        bool HasFlag(const UIFlags flag) const { return Flags & flag; }
        void AddFlag(const UIFlags flag) { Flags |= flag; }
        void RemoveFlag(const UIFlags flag) { Flags &= ~flag; }
    private:
        uint8_t Flags = VISIBLE | ENABLED;
    };
} // namespace UI
