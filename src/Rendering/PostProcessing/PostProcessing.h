#pragma once
#include "Rendering/Types/FBO/FrameBuffer.h"
#include "Scripting/SmallFunction.h"
#include <memory>
#include <vector>

namespace Rendering {
    class Shader;
}

namespace Rendering::PostProcessing {

    void DrawFullscreenTriangle();

    struct PostEffect {
        std::shared_ptr<Shader> shader;
        bool enabled = true;
        float strength = 1.0f;
        // optionally set effect specific uniforms
        Scripting::SmallFunction<void(Shader &)> bindUniforms;
    };

    class PostProcessor {
    public:
        void AddEffect(PostEffect fx) { m_Effects.push_back(std::move(fx)); }
        FrameBuffer *Execute(FrameBuffer *scene, FrameBuffer *pingA, FrameBuffer *pingB);

    private:
        std::vector<PostEffect> m_Effects;
    };

} // namespace Rendering::PostProcessing
