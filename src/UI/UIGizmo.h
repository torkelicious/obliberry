#pragma once
#include "UIElement.h"
#include "Rendering/UIRenderer.h"
#include "Rendering/UISystem.h"
namespace UI {

    // "gizmo"s
    constexpr glm::vec4 elOutline = {0.0f, 0.8f, 1.0f, 1.0f};
    constexpr glm::vec4 elHandle = {1.0f, 1.0f, 1.0f, 1.0f};
    constexpr glm::vec4 elHandleHover = {0.3f, 1.0f, 0.5f, 1.0f};
    constexpr glm::vec4 elBorder = {0.1f, 0.1f, 0.1f, 1.0f};
    constexpr float lineThickness = 2.0f;
    constexpr float handleSize = 12.0f;
    constexpr float handleHitPad = 4.0f;

    enum class HandleType : uint8_t { None, Translate, TL, TC, TR, RC, BR, BC, BL, LC };

    inline glm::vec2 GetWorldPosition(const UIElement *element) {
        glm::vec2 worldPos = element->Rect.Position;
        const UI::UIElement *current = element->Parent;
        while (current != nullptr) {
            worldPos += current->Rect.Position;
            current = current->Parent;
        }
        return worldPos;
    }

    inline HandleType HitTest(const glm::vec2 &mPos, const UIElement *element) {
        const glm::vec2 worldPos = GetWorldPosition(element);
        const glm::vec2 size = element->Rect.Scale;
        constexpr float half = (handleSize + handleHitPad) * 0.5f;
        constexpr float hitSize = handleSize + handleHitPad;

        const std::pair<HandleType, RectTransform> handles[8] = {

                {HandleType::TL, {{worldPos.x - half, worldPos.y - half}, {hitSize, hitSize}}},
                {HandleType::TC, {{worldPos.x + size.x * 0.5f - half, worldPos.y - half}, {hitSize, hitSize}}},
                {HandleType::TR, {{worldPos.x + size.x - half, worldPos.y - half}, {hitSize, hitSize}}},
                {HandleType::RC, {{worldPos.x + size.x - half, worldPos.y + size.y * 0.5f - half}, {hitSize, hitSize}}},
                {HandleType::BR, {{worldPos.x + size.x - half, worldPos.y + size.y - half}, {hitSize, hitSize}}},
                {HandleType::BC, {{worldPos.x + size.x * 0.5f - half, worldPos.y + size.y - half}, {hitSize, hitSize}}},
                {HandleType::BL, {{worldPos.x - half, worldPos.y + size.y - half}, {hitSize, hitSize}}},
                {HandleType::LC, {{worldPos.x - half, worldPos.y + size.y * 0.5f - half}, {hitSize, hitSize}}}};

        for (const auto &[type, rect] : handles) {
            if (IsPointInsideRect(mPos, rect))
                return type;
        }

        if (IsPointInsideRect(mPos, {worldPos, size}))
            return HandleType::Translate;

        return HandleType::None;
    }

    inline void TransformElement(UIElement *element, const HandleType type, const glm::vec2 &delta, const glm::vec2 &initialWorldPos, const glm::vec2 &initialScale, const float minSize = 10.0f) {
        if (!element || type == HandleType::None)
            return;

        const float maxDeltaL = initialScale.x - minSize;
        const float maxDeltaT = initialScale.y - minSize;
        const float clampDeltaX = std::min(delta.x, maxDeltaL);
        const float clampDeltaY = std::min(delta.y, maxDeltaT);

        glm::vec2 targetWorld = initialWorldPos;
        glm::vec2 targetScale = initialScale;

        switch (type) {
            case HandleType::Translate:
                targetWorld += delta;
                break;
            case HandleType::TL:
                targetWorld += glm::vec2(clampDeltaX, clampDeltaY);
                targetScale -= glm::vec2(clampDeltaX, clampDeltaY);
                break;
            case HandleType::TC:
                targetWorld.y += clampDeltaY;
                targetScale.y -= clampDeltaY;
                break;
            case HandleType::TR:
                targetScale.x = std::max(minSize, initialScale.x + delta.x);
                targetWorld.y += clampDeltaY;
                targetScale.y -= clampDeltaY;
                break;
            case HandleType::RC:
                targetScale.x = std::max(minSize, initialScale.x + delta.x);
                break;
            case HandleType::BR:
                targetScale = glm::max(glm::vec2(minSize), initialScale + delta);
                break;
            case HandleType::BC:
                targetScale.y = std::max(minSize, initialScale.y + delta.y);
                break;
            case HandleType::BL:
                targetWorld.x += clampDeltaX;
                targetScale.x -= clampDeltaX;
                targetScale.y = std::max(minSize, initialScale.y + delta.y);
                break;
            case HandleType::LC:
                targetWorld.x += clampDeltaX;
                targetScale.x -= clampDeltaX;
                break;
            default:
                break;
        }
        element->Rect.Scale = targetScale;
        const glm::vec2 parentWorld = element->Parent ? GetWorldPosition(element->Parent) : glm::vec2(0.0f);
        element->Rect.Position = targetWorld - parentWorld;
    }


    inline void DrawGizmo(const UIElement *element, UIRenderer *renderer, const HandleType hoveredHandle = HandleType::None) {
        if (!renderer || !element) {
            return;
        }
        const glm::vec2 worldPos = GetWorldPosition(element);
        const glm::vec2 size = element->Rect.Scale;


        // selection bounding box
        renderer->SubmitRect(worldPos, {size.x, lineThickness}, elOutline);
        renderer->SubmitRect({worldPos.x, worldPos.y + size.y - lineThickness}, {size.x, lineThickness}, elOutline);
        renderer->SubmitRect(worldPos, {lineThickness, size.y}, elOutline);
        renderer->SubmitRect({worldPos.x + size.x - lineThickness, worldPos.y}, {lineThickness, size.y}, elOutline);

        // handle anchor thingys
        const std::pair<HandleType, glm::vec2> handles[8] = {
                {HandleType::TL, worldPos},                                    // Top-Left
                {HandleType::TC, worldPos + glm::vec2(size.x * 0.5f, 0.0f)},   // Top-Center
                {HandleType::TR, worldPos + glm::vec2(size.x, 0.0f)},          // Top-Right
                {HandleType::RC, worldPos + glm::vec2(size.x, size.y * 0.5f)}, // Right-Center
                {HandleType::BR, worldPos + size},                             // Bottom-Right
                {HandleType::BC, worldPos + glm::vec2(size.x * 0.5f, size.y)}, // Bottom-Center
                {HandleType::BL, worldPos + glm::vec2(0.0f, size.y)},          // Bottom-Left
                {HandleType::LC, worldPos + glm::vec2(0.0f, size.y * 0.5f)}    // Left-Center
        };

        for (const auto &[type, center] : handles) {
            const glm::vec4 &fillColor = type == hoveredHandle ? elHandleHover : elHandle;
            // border
            const glm::vec2 handlePos = center - glm::vec2(handleSize * 0.5f);
            renderer->SubmitRect(handlePos - glm::vec2(1.0f), {handleSize + 2.0f, handleSize + 2.0f}, elBorder);
            // fill
            renderer->SubmitRect(handlePos, {handleSize, handleSize}, fillColor);
        }
    }
} // namespace UI
