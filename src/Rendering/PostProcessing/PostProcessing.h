#pragma once
#include "Core/ResourceManager.h"
#include "Rendering/Types/FBO/FrameBuffer.h"
#include <glm/glm.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Rendering {
    class Shader;
}

namespace Rendering::PostProcessing {

    void DrawFullscreenTriangle();

    // serializable uniform value
    // types map to Shader::SetUniform* overloads.
    // vec types are written as JSON arrays
    using UniformValue = std::variant<float, int, glm::vec2, glm::vec3, glm::vec4>;

    // every one automatically receives these engine fed uniforms
    //   uniform sampler2D u_Texture   : the previous pass output
    //   uniform vec2      resolution  : render target size in px
    //   uniform float     time        : seconds since engine start
    inline constexpr const char *kUniformResolution = "resolution";
    inline constexpr const char *kUniformTime = "time";

    struct PostEffect {
        std::string shaderKey; // ResourceManager key, e.g. "[Engine_PP] CRT"
        bool enabled = true;
        std::unordered_map<std::string, UniformValue> uniforms;
        std::shared_ptr<Shader> shader; // runtime cache resolved from shaderKey
        void ResolveShader();           // fetch by shaderKey
    };

    class PostProcessor {
    public:
        void AddEffect(PostEffect fx);
        [[nodiscard]] std::vector<PostEffect> &Effects() { return m_Effects; }

        FrameBuffer *Execute(FrameBuffer *scene, FrameBuffer *pingA, FrameBuffer *pingB);
        // JSON :
        // [
        //   { "shader": "[Engine_PP] Grayscale", "enabled": false,
        //     "uniforms": { "u_Strength": 1.0 } },
        //   { "shader": "[Engine_PP] CRT", "enabled": true,
        //     "uniforms": { "jitter": 0.001, "maskSize": [1.0, 1.0] } }
        // ]
        [[nodiscard]] nlohmann::json Serialize() const;
        bool Deserialize(const nlohmann::json &j);

    private:
        // effects run in vector order
        std::vector<PostEffect> m_Effects;
    };

} // namespace Rendering::PostProcessing
