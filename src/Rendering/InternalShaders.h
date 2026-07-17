#pragma once

// Obliberry Engine builtin shader sources
// These are compiled into the binary so they
// do not need to interact with the
// filesystem / VFS and cannot be accidentally deleted by users
//
// Base shaders      default shader used for all meshses, map, entities, etc
// Particle shaders  per-instance color for particle emitters
// Light shaders     additive point light accumulation into a lightmap FBO

#include <memory>
#include "Core/ResourceManager.h"
#include "Rendering/Shader.h"

namespace Rendering::BuiltinShaders {

    // Base shaders

    inline constexpr char kBaseVert[] = R"(
#version 330 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec2 a_UV;
layout(location = 2) in mat4 a_InstanceMatrix;
layout(location = 6) in int a_EntityID;

uniform mat4 u_VP;
uniform vec2 u_MapSize;
uniform vec2 u_MapOffset;

out vec2 v_UV;
out vec2 v_LightUV;
flat out int v_EntityID;

void main()
{
    v_UV = a_UV;

    vec4 worldPos = a_InstanceMatrix * vec4(a_Pos, 1.0);

    // Convert world coordinates to lightmap coordinates
    v_LightUV = (worldPos.xy - u_MapOffset) / u_MapSize;

    gl_Position = u_VP * worldPos;
    v_EntityID = a_EntityID;
}
)";

    inline constexpr char kBaseFrag[] = R"(
#version 330 core

in vec2 v_UV;
in vec2 v_LightUV;

uniform sampler2D u_Texture;
uniform sampler2D u_LightTexture;

uniform vec4 u_Color;
uniform float u_Ambient;
flat in int v_EntityID;
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int OutEntityID;

void main()
{
    vec4 tex = texture(u_Texture, v_UV);

    float finalAlpha = tex.a * u_Color.a;

    if (finalAlpha < 0.01) {
        discard;
    }

    // RGB lightmap
    vec3 light = texture(u_LightTexture, v_LightUV).rgb;
    // Prevent total darkness
    light = max(light, vec3(u_Ambient));

    vec3 finalColor = tex.rgb * u_Color.rgb * light;

    FragColor = vec4(finalColor, finalAlpha);

    // Write the entity ID from the instanced vertex attribute
    OutEntityID = v_EntityID;
}
)";

    // Particle shaders , per-instance color

    inline constexpr char kParticleVert[] = R"(
#version 330 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec2 a_UV;
layout(location = 2) in mat4 a_InstanceMatrix;
layout(location = 6) in int a_EntityID;
layout(location = 7) in vec4 a_InstanceColor;

uniform mat4 u_VP;
uniform vec2 u_MapSize;
uniform vec2 u_MapOffset;

out vec2 v_UV;
out vec2 v_LightUV;
out vec4 v_InstanceColor;
flat out int v_EntityID;

void main()
{
    v_UV = a_UV;
    v_InstanceColor = a_InstanceColor;
    vec4 worldPos = a_InstanceMatrix * vec4(a_Pos, 1.0);
    v_LightUV = (worldPos.xy - u_MapOffset) / u_MapSize;
    gl_Position = u_VP * worldPos;
    v_EntityID = a_EntityID;
}
)";

    inline constexpr char kParticleFrag[] = R"(
#version 330 core

in vec2 v_UV;
in vec2 v_LightUV;
in vec4 v_InstanceColor;

uniform sampler2D u_Texture;
uniform sampler2D u_LightTexture;
uniform float u_Ambient;
flat in int v_EntityID;
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int OutEntityID;

void main()
{
    vec4 tex = texture(u_Texture, v_UV);
    float finalAlpha = tex.a * v_InstanceColor.a;
    if (finalAlpha < 0.01) discard;
    vec3 light = texture(u_LightTexture, v_LightUV).rgb;
    light = max(light, vec3(u_Ambient));
    vec3 finalColor = tex.rgb * v_InstanceColor.rgb * light;
    FragColor = vec4(finalColor, finalAlpha);
    OutEntityID = v_EntityID;
}
)";

    // Lighting shaders
    inline constexpr char kLightVert[] = R"(
#version 330 core
layout (location = 0) in vec2 a_pos;

uniform mat4 u_projection;
uniform mat4 u_model;

out vec2 v_uv;

void main() {
    v_uv = a_pos * 2.0;
    gl_Position = u_projection * u_model * vec4(a_pos, 0.0, 1.0);
}
)";

    inline constexpr char kLightFrag[] = R"(
#version 330 core
in vec2 v_uv;
out vec4 FragColor;

uniform vec3 u_color;
uniform float u_intensity;

void main() {
    float distSq = dot(v_uv, v_uv);

    if (distSq > 1.0) {
        discard;
    }

    float falloff = 1.0 - distSq;

    FragColor = vec4(u_color * u_intensity * falloff, 1.0);
}
)";

    //
    // Registration
    //

    // Call once after GL context is current.
    inline void RegisterBuiltinShaders(Core::ResourceManager &resources) {
        // Base shader , default shader used for all meshes, map, entities, etc.
        auto baseShader = std::make_shared<Shader>(kBaseVert, kBaseFrag, "<base>");
        baseShader->InitGL();
        resources.LoadFromFactory<Shader>("[Engine] Base", [baseShader] { return baseShader; });

        // Particle shader , per-instance color for particle emitters
        auto particleShader = std::make_shared<Shader>(kParticleVert, kParticleFrag, "<particle>");
        particleShader->InitGL();
        resources.LoadFromFactory<Shader>("[Engine] Particle", [particleShader] { return particleShader; });

        // Light shader , additive point-light accumulation into a lightmap FBO
        auto lightShader = std::make_shared<Shader>(kLightVert, kLightFrag, "<light>");
        lightShader->InitGL();
        resources.LoadFromFactory<Shader>("[Engine] Light", [lightShader] { return lightShader; });
    }

} // namespace Rendering::BuiltinShaders
