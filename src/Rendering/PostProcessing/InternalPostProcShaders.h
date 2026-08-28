#pragma once
#include "Core/ResourceManager.h"
#include "Rendering/Types/Shader/Shader.h"

// same idea as src/Rendering/Types/Shader/InternalShaders.h

namespace Rendering::PostProcessing::BuiltinShaders {
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


    inline void RegisterBuiltinPostProcShaders(Core::ResourceManager &resources) {
        resources.LoadFromFactory<Shader>("[Engine_PP] Passthrough", [] {
            auto passthroughShader = std::make_shared<Shader>(kPP_PassthroughShaderVert, kPP_PassthroughShaderFrag, "<PP_Passthrough>");
            passthroughShader->InitGL();
            return passthroughShader;
        });
    }

} // namespace Rendering::PostProcessing::BuiltinShaders
