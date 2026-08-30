#pragma once
#include "Core/ResourceManager.h"
#include "PostProcessing.h"
#include "Rendering/Renderer.h"
#include "Rendering/Types/Shader/Shader.h"
#include <glm/glm.hpp>

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

    // heavily inspired by various posts on shadertoy and some of retroarch's shader sources
    inline constexpr char kPP_CRTShaderFrag[] = R"(
#version 330 core
in vec2 v_UV;
out vec4 FragColor;

uniform sampler2D u_Texture;
uniform vec2 resolution;
uniform float time;

// all tunable via the serialized uniform bag (0 => feature off)
uniform float u_Curvature;   // barrel distortion strength
uniform float u_Aberration;  // chromatic aberration at edges
uniform float u_Scanline;    // scanline intensity 0..1
uniform float u_Mask;        // slot/aperture mask intensity 0..1
uniform float u_Glow;        // phosphor glow bleed
uniform float u_Noise;       // static noise
uniform float u_Flicker;     // frame brightness flicker 0..1
uniform float u_Vignette;    // corner darkening

float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

// barrel distortion
vec2 curveUV(vec2 uv) {
    vec2 p = uv * 2.0 - 1.0;
    p *= 1.0 + u_Curvature * dot(p, p);
    return p * 0.5 + 0.5;
}

void main() {
    vec2 uv = curveUV(v_UV);

    // outside the curved tube
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec2 px = uv * resolution; // integer pixel coordinates

    // slight horizontal wobble per scanline, stepped like a bad signal
    float row = floor(px.y);
    float jitter = (hash(vec2(row, floor(time * 24.0))) - 0.5) * 0.0015;
    uv.x += jitter;

    // chromatic aberration
    vec2 centered = uv * 2.0 - 1.0;
    vec2 ab = centered * (u_Aberration * dot(centered, centered));

    // slot mask
    float cellU = fract(px.x / 3.0);
    float band = smoothstep(0.18, 0.38, cellU) * (1.0 - smoothstep(0.72, 0.92, cellU));
    float col = mod(floor(px.x), 3.0);
    vec3 mask = vec3(0.0);
    if      (col < 1.0) mask.r = band;
    else if (col < 2.0) mask.g = band;
    else                mask.b = band;

    // vertical aperture
    float apV = smoothstep(0.12, 0.45, fract(px.y)) * (1.0 - smoothstep(0.65, 0.98, fract(px.y)));
    mask *= 0.55 + 0.45 * apV;

    // sample the image per channel so the mask phosphors pull apart slightly
    vec3 color;
    color.r = texture(u_Texture, uv + ab).r;
    color.g = texture(u_Texture, uv).g;
    color.b = texture(u_Texture, uv - ab).b;

    // phosphor bleed
    float texel = 1.0 / resolution.y;
    color += 0.22 * texture(u_Texture, uv + vec2(0.0,  texel)).rgb;
    color += 0.22 * texture(u_Texture, uv - vec2(0.0,  texel)).rgb;
    color /= 1.44;

    // scanlines
    float scan = fract(px.y * 0.5) < 0.5 ? 0.62 : 1.0;
    color *= mix(1.0, scan, u_Scanline);

    // faint brightline sweeping slowly down the tube
    float sweep = fract(time * 0.9);
    float brightline = smoothstep(0.0, 0.02, uv.y - (sweep - 0.03)) * (1.0 - smoothstep(0.0, 0.02, uv.y - sweep));
    color += brightline * 0.08;

    color *= mix(vec3(1.0), mask * 2.6, u_Mask);        // mask darkens, boost compensates
    float lum = dot(color, vec3(0.299, 0.587, 0.114));
    color += color * smoothstep(0.35, 0.95, lum) * u_Glow; // phosphor glow

    // grain + noise band
    float grain = (hash(px + floor(time * 60.0)) - 0.5) * u_Noise;
    float bandPos = fract(time * 0.35);
    float rolling = smoothstep(0.0, 0.06, uv.y - bandPos) * (1.0 - smoothstep(0.0, 0.06, uv.y - bandPos));
    color += grain + rolling * 0.05 * u_Noise;
    color *= 1.0 + (hash(vec2(floor(time * 120.0), 3.7)) - 0.5) * 0.015 * u_Flicker;

    // vignette
    color *= clamp(1.0 - dot(centered, centered) * u_Vignette, 0.0, 1.0);

    FragColor = vec4(color, 1.0);
}
)";


    // registration
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

        LoadPPShader(resources, "Grayscale", kPP_PassthroughShaderVert, kPP_GrayscaleShaderFrag);
        {
            PostEffect fx;
            fx.shaderKey = "[Engine_PP] Grayscale";
            fx.enabled = false;
            fx.uniforms["u_Strength"] = 1.0f;
            postproc.AddEffect(std::move(fx));
        }

        LoadPPShader(resources, "CRT", kPP_PassthroughShaderVert, kPP_CRTShaderFrag);
        {
            PostEffect fx;
            fx.shaderKey = "[Engine_PP] CRT";
            fx.enabled = true;
            fx.uniforms = {
                    {"u_Curvature", 0.05f}, {"u_Aberration", 0.0015f}, {"u_Scanline", 0.7f}, {"u_Mask", 0.25f}, {"u_Glow", 0.25f}, {"u_Noise", 0.03f}, {"u_Flicker", 0.5f}, {"u_Vignette", 0.15f},
            };
            postproc.AddEffect(std::move(fx));
        }
    }
} // namespace Rendering::PostProcessing::Builtins
