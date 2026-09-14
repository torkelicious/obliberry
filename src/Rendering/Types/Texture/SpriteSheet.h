#pragma once

#include "Rendering/Types/Texture/Texture.h"
#include <memory>
#include <glm/glm.hpp>

namespace Rendering {

    struct SpriteSheet {
        std::shared_ptr<Texture> texture;
        int columns = 1;
        int rows = 1;

        [[nodiscard]] int FrameCount() const noexcept { return columns * rows; }

        // { offsetU, offsetV, scaleU, scaleV }
        [[nodiscard]] glm::vec4 GetFrameUV(int frame) const noexcept {
            if (columns <= 0 || rows <= 0)
                return {0.0f, 0.0f, 1.0f, 1.0f};

            frame = frame % FrameCount();
            if (frame < 0)
                frame += FrameCount();

            const int col = frame % columns;
            const int row = frame / columns;

            const float scaleU = 1.0f / static_cast<float>(columns);
            const float scaleV = 1.0f / static_cast<float>(rows);
            const float offsetV = 1.0f - static_cast<float>(row + 1) * scaleV;

            return {static_cast<float>(col) * scaleU, offsetV, scaleU, scaleV};
        }
    };

} // namespace Rendering
