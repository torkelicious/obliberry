#pragma once


#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "Texture.h"

namespace Rendering {
    struct Lightmap {
        std::vector<glm::vec3> accumulationBuffer;
        std::vector<unsigned char> pixelBuffer;
        std::shared_ptr<Texture> texture;
        glm::vec2 mapOffset = {0.0f, 0.0f};
        glm::vec2 mapSize = {1000.0f, 1000.0f};
        float ambient = 0.2f;
        size_t lastLightCount = 0;
    };
} // namespace Rendering
