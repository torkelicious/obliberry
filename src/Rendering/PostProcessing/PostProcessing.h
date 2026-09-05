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

    // uniform value vec types as JSON arrays
    // boolk map to int (0/1)
    using UniformValue = std::variant<float, int, glm::vec2, glm::vec3, glm::vec4>;

    // engine uniforms every one receives
    //   uniform sampler2D u_Texture     : previous pass output     (0)
    //   uniform vec2      u_Resolution  : render target size in px
    //   uniform vec2      u_TexelSize   : 1 / u_Resolution
    //   uniform float     u_Time        : seconds since engine start
    //   uniform sampler2D u_Scene       : original scene texture   (1, only when wantsSceneTexture)
    inline constexpr auto kUniformResolution = "u_Resolution";
    inline constexpr auto kUniformTexelSize = "u_TexelSize";
    inline constexpr auto kUniformTime = "u_Time";
    inline constexpr auto kUniformScene = "u_Scene";

    struct PostEffect {
        std::string shaderKey; // ResourceManager key, e.g. "[Engine_PP] CRT"
        bool enabled = true;
        std::unordered_map<std::string, UniformValue> uniforms; // serialized tunables

        // multi pass support
        // the effect runs "passes" times, ping pong
        int passes = 1;
        std::vector<std::unordered_map<std::string, UniformValue>> passUniforms;
        bool wantsSceneTexture = false;
        std::shared_ptr<Shader> shader;
        void ResolveShader(); // fetch by shaderKey
    };

    class PostProcessor {
    public:
        void AddEffect(PostEffect fx);
        [[nodiscard]] std::vector<PostEffect> &Effects() { return m_Effects; }

        FrameBuffer *Execute(FrameBuffer *scene, FrameBuffer *pingA, FrameBuffer *pingB);

        // JSON:
        // [
        //   { "shader": "[Engine_PP] Grayscale", "enabled": false,
        //     "uniforms": { "u_Strength": 1.0 } },
        //   { "shader": "[Engine_PP] GaussianBlur", "passes": 2,
        //     "passUniforms": [ { "u_Horizontal": 1 }, { "u_Horizontal": 0 } ] },
        //   { "shader": "[Engine_PP] Composite", "wantsSceneTexture": true,
        //     "uniforms": { "u_BloomStrength": 1.0 } }
        // ]
        [[nodiscard]] nlohmann::json Serialize() const;
        bool Deserialize(const nlohmann::json &j);

    private:
        // effects run in vector order
        std::vector<PostEffect> m_Effects;
    };

    [[nodiscard]] nlohmann::json SerializeEffects(const std::vector<PostEffect> &effects);

} // namespace Rendering::PostProcessing
