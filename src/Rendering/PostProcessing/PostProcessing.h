#pragma once
#include "Rendering/Types/FBO/FrameBuffer.h"
#include <memory>
#include <vector>

namespace Rendering::PostProcessing {

    class PostProcessEffect {
    public:
        virtual ~PostProcessEffect() = default;
        virtual void Apply(uint32_t inputColTex, FrameBuffer *output) = 0;
        bool enabled = true;

        static void DrawFullscreenTriangle();
    };

    class PostProcessor {
    public:
        void AddEffect(std::unique_ptr<PostProcessEffect> fx) { m_Effects.push_back(std::move(fx)); }
        FrameBuffer *Execute(uint32_t sceneColTex, FrameBuffer *pingA, FrameBuffer *pingB) const;

    private:
        std::vector<std::unique_ptr<PostProcessEffect>> m_Effects;
    };

} // namespace Rendering::PostProcessing
