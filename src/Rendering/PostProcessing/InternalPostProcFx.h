#pragma once
#include "Core/ResourceManager.h"
#include "Rendering/Types/Shader/Shader.h"

// same idea as src/Rendering/Types/Shader/InternalShaders.h


namespace Rendering::PostProcessing::Builtins {

    inline constexpr char kPP_PassthroughShaderVert[] = R"(
#version 330 core
out vec2 v_UV;
void main() {
    vec2 pos = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    v_UV = pos;
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}
)";


    inline constexpr char kPP_PassthroughShaderFrag[] = R"(
#version 330 core
in vec2 v_UV;
out vec4 FragColor;
uniform sampler2D u_Texture;
void main() {
    FragColor = texture(u_Texture, v_UV);
}
)";


    inline constexpr char kPP_GrayscaleShaderFrag[] = R"(
#version 330 core
in vec2 v_UV;
out vec4 FragColor;
uniform sampler2D u_Texture;
uniform float u_Strength;
void main() {
    vec4 c = texture(u_Texture, v_UV);
    float gray = dot(c.rgb, vec3(0.2126, 0.7152, 0.0722));
    FragColor = vec4(mix(c.rgb, vec3(gray), u_Strength), c.a);
}
)";


    inline std::vector<ShaderRegistration> shaderRegistrations = {
            {.name = "Passthrough", .vertex = kPP_PassthroughShaderVert, .fragment = kPP_PassthroughShaderFrag},
            {.name = "Greyscale", .vertex = kPP_PassthroughShaderVert, .fragment = kPP_GrayscaleShaderFrag},
    };

    struct FxRegistrationKey {
        std::string shaderName; // key is deduced from name
        PostEffect fx;
        std::string shaderKey() const { return "[Engine_PP] " + shaderName; }
    };

    inline std::vector<FxRegistrationKey> fxRegistrations = {

            {.shaderName = "Greyscale", .fx = {.type = PostEffectType::Grayscale, .enabled = true, .strength = 1.0f}}

    };


    inline void RegisterBuiltinPostProcShaders(Rendering::Renderer &renderer) {
        auto &resources = Core::ResourceManager::GetInstance();
        auto &postproc = renderer.GetPostProcessor();

        for (const auto &shad : shaderRegistrations) {
            const std::string resourceKey = "[Engine_PP] " + shad.name;
            const std::string debugName = "<PP_" + shad.name + ">";
            resources.LoadFromFactory<Shader>(resourceKey, [shad, debugName] {
                auto shader = std::make_shared<Shader>(std::string(shad.vertex), std::string(shad.fragment), debugName);
                shader->InitGL();
                return shader;
            });
        }

        for (const auto &registration : fxRegistrations) {
            postproc.RegisterShader(registration.fx.type, resources.Get<Shader>(registration.shaderKey()));
            postproc.AddEffect(registration.fx);
        }
    }
} // namespace Rendering::PostProcessing::Builtins
