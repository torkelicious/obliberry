#version 330 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec2 a_UV;
layout(location = 2) in mat4 a_InstanceMatrix;

uniform mat4 u_VP;

out vec2 v_UV;

void main() {
    v_UV = a_UV; // Pass the UVs through

    // we will troll opengl and send non instanced matrixes still to this..... ;)
    gl_Position = u_VP * a_InstanceMatrix * vec4(a_Pos, 1.0);
}
