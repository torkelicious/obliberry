#version 330 core
layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_UV;

uniform mat4 u_Model;
uniform mat4 u_ViewProjection; // projection matrix

out vec2 v_UV;

void main() {
    gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 0.0, 1.0);
    v_UV = a_UV;
}
