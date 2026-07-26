#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <limits>
#include "FrameBuffer.h"
#include "Mesh.h"
#include "Shader.h"
#include "VertexArray.h"

namespace Rendering {
    struct Lightmap {
        std::shared_ptr<FrameBuffer> framebuffer;
        std::shared_ptr<Mesh> lightQuad;
        std::shared_ptr<Shader> lightShader;
        std::shared_ptr<VertexArray> lightQuadVAO;
        glm::vec2 mapOffset{0.0f};
        glm::vec2 mapSize{0.0f};
        float ambient = 0.1f;
        size_t lastLightCount = std::numeric_limits<size_t>::max();
    };

} // namespace Rendering
