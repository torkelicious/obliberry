#pragma once
#include "Core/ResourceManager.h"
#include "PostProcessing.h"
#include "Rendering/Renderer.h"
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

    //
    // reg
    //

    struct FxRegistration {
        std::string name; // resource key becomes "[Engine_PP] <name>"
        const char *vertex;
        const char *fragment;
        bool enabled = true;
        float strength = 1.0f;
    };

    inline std::vector<FxRegistration> fxRegistrations = {
            {.name = "Grayscale", .vertex = kPP_PassthroughShaderVert, .fragment = kPP_GrayscaleShaderFrag, .enabled = false, .strength = 1.0f},
    };

    inline std::shared_ptr<Shader> LoadPPShader(Core::ResourceManager &resources, const std::string &name, const char *vert, const char *frag) {
        const std::string resourceKey = "[Engine_PP] " + name;
        const std::string debugName = "<PP_" + name + ">";
        return resources.LoadFromFactory<Shader>(resourceKey, [vert, frag, debugName] {
            auto shader = std::make_shared<Shader>(std::string(vert), std::string(frag), debugName);
            shader->InitGL();
            return shader;
        });
    }

    inline void RegisterBuiltinPostProcShaders(Rendering::Renderer &renderer) {
        auto &resources = Core::ResourceManager::GetInstance();
        auto &postproc = renderer.GetPostProcessor();

        renderer.SetPassthroughShader(LoadPPShader(resources, "Passthrough", kPP_PassthroughShaderVert, kPP_PassthroughShaderFrag));

        for (const auto &reg : fxRegistrations) {
            postproc.AddEffect({.shader = LoadPPShader(resources, reg.name, reg.vertex, reg.fragment), .enabled = reg.enabled, .strength = reg.strength});
        }
    }

} // namespace Rendering::PostProcessing::Builtins
