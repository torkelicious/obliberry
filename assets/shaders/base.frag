#version 330 core

in vec2 v_UV;
in vec2 v_LightUV;

uniform sampler2D u_Texture;
uniform sampler2D u_LightTexture;

uniform vec4 u_Color;
uniform float u_Ambient;
out vec4 FragColor;

void main()
{
    vec4 tex = texture(u_Texture, v_UV);

    float finalAlpha = tex.a * u_Color.a;

    if (finalAlpha < 0.05) {
        discard;
    }

    // RGB lightmap
    vec3 light = texture(u_LightTexture, v_LightUV).rgb;
    // Prevent total darkness
    light = max(light, vec3(u_Ambient));

    vec3 finalColor = tex.rgb * u_Color.rgb * light;

    FragColor = vec4(finalColor, finalAlpha);
}
