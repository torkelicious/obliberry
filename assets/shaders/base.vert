#version 330 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec2 a_UV;
layout(location = 2) in mat4 a_InstanceMatrix;

uniform mat4 u_VP;
uniform vec2 u_MapSize;
uniform vec2 u_MapOffset;

out vec2 v_UV;
out vec2 v_LightUV;

void main()
{
    v_UV = a_UV;

    vec4 worldPos = a_InstanceMatrix * vec4(a_Pos, 1.0);

    // Convert world coordinates to lightmap coordinates
    v_LightUV = (worldPos.xy - u_MapOffset) / u_MapSize;

    gl_Position = u_VP * worldPos;
}
