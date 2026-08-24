#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <limits>
#include <vector>
#include "FrameBuffer.h"
#include "Mesh.h"
#include "Shader.h"
#include "VertexArray.h"

namespace Rendering {
    // one packed light as uploaded to the lightmap
    struct GPULight {
        float x, y;
        float radius;
        float colorR, colorG, colorB;

        bool operator==(const GPULight &) const = default;
    };

    struct Lightmap {
        std::shared_ptr<FrameBuffer> framebuffer;
        std::shared_ptr<Mesh> lightQuad;
        std::shared_ptr<Shader> lightShader;
        std::shared_ptr<VertexArray> lightQuadVAO;
        glm::vec2 mapOffset{0.0f};
        glm::vec2 mapSize{0.0f};
        float ambient = 0.1f;
        size_t lastLightCount = std::numeric_limits<size_t>::max();
        // last rendered light state, used to detect changes
        std::vector<GPULight> lastPackedLights;
        std::vector<GPULight> scratchPackedLights;
    };

} // namespace Rendering
