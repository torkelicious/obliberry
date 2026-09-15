#pragma once

#include "Rendering/Types/Texture/Texture.h"
#include <memory>
#include <glm/glm.hpp>

namespace Rendering {

    struct SpriteSheet {
        std::shared_ptr<Texture> texture;
        int columns = 1;
        int rows = 1;

        // padding
        int columnSpacing = 0;
        int rowSpacing = 0;

        [[nodiscard]] int FrameCount() const noexcept { return columns * rows; }

        // { offsetU, offsetV, scaleU, scaleV }
        [[nodiscard]] glm::vec4 GetFrameUV(int frame) const noexcept {
            if (!texture || columns <= 0 || rows <= 0 || columnSpacing < 0 || rowSpacing < 0) {
                return {0.0f, 0.0f, 1.0f, 1.0f};
            }

            const int textureWidth = texture->GetWidth();
            const int textureHeight = texture->GetHeight();

            if (textureWidth <= 0 || textureHeight <= 0)
                return {0.0f, 0.0f, 1.0f, 1.0f};

            const int totalHorizontalSpacing = columnSpacing * (columns - 1);
            const int totalVerticalSpacing = rowSpacing * (rows - 1);

            const int availableWidth = textureWidth - totalHorizontalSpacing;
            const int availableHeight = textureHeight - totalVerticalSpacing;

            if (availableWidth <= 0 || availableHeight <= 0)
                return {0.0f, 0.0f, 1.0f, 1.0f};

            if (availableWidth % columns != 0 || availableHeight % rows != 0) {
                return {0.0f, 0.0f, 1.0f, 1.0f};
            }

            const int frameWidth = availableWidth / columns;
            const int frameHeight = availableHeight / rows;

            frame %= FrameCount();
            if (frame < 0)
                frame += FrameCount();

            const int column = frame % columns;
            const int row = frame / columns;

            const int pixelX = column * (frameWidth + columnSpacing);
            const int pixelY = row * (frameHeight + rowSpacing);

            const float offsetU = static_cast<float>(pixelX) / static_cast<float>(textureWidth);
            const float offsetV = 1.0f - static_cast<float>(pixelY + frameHeight) / static_cast<float>(textureHeight);
            const float scaleU = static_cast<float>(frameWidth) / static_cast<float>(textureWidth);
            const float scaleV = static_cast<float>(frameHeight) / static_cast<float>(textureHeight);
            return {offsetU, offsetV, scaleU, scaleV};
        }
    };
} // namespace Rendering
