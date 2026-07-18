#pragma once
#include <glm/glm.hpp>
namespace UI {
    struct RectTransform {
        glm::vec2 Position;
        glm::vec2 Scale;

        // For future implementation
        // so i dont forgor
        // glm::vec2 Pivot;
        // glm::vec2 AnchorMin;
        // glm::vec2 AnchorMax;
    };
} // namespace UI
