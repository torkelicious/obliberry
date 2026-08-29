#pragma once
#include "Core/ResourceManager.h"
#include "Rendering/Types/FBO/FrameBuffer.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace Rendering {
    class Shader;
}

namespace Rendering::PostProcessing {

    void DrawFullscreenTriangle();

    enum class PostEffectType : uint8_t {
        Grayscale,
    };

    struct PostEffect {
        PostEffectType type = PostEffectType::Grayscale;
        bool enabled = true;
        float strength = 1.0f; // grayscale: 0 = full color, 1 = grayscale
    };

    class PostProcessor {
    public:
        void RegisterShader(PostEffectType type, std::shared_ptr<Shader> shader) { m_Shaders[type] = std::move(shader); }

        void AddEffect(const PostEffect &fx) { m_Effects.push_back(fx); }

        FrameBuffer *Execute(FrameBuffer *scene, FrameBuffer *pingA, FrameBuffer *pingB) const;

    private:
        void BindUniforms(const PostEffect &fx, Shader &shader) const;
        [[nodiscard]] Shader *FindShader(PostEffectType type) const;

        std::vector<PostEffect> m_Effects;
        std::unordered_map<PostEffectType, std::shared_ptr<Shader>> m_Shaders;
    };

} // namespace Rendering::PostProcessing
