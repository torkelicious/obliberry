#pragma once

namespace UI {

    //
    // Obliberry builtin internal UI shaders
    //

    inline constexpr char kUIVertShader[] = R"(
#version 330 core
layout(location=0) in vec2 a_Pos;
layout(location=1) in vec2 a_UV;
layout(location=2) in vec4 a_Color;
uniform mat4 u_Projection;
out vec2 v_UV;
out vec4 v_Color;
void main() {
    v_UV = a_UV;
    v_Color = a_Color;
    gl_Position = u_Projection * vec4(a_Pos, 0.0, 1.0);
}
)";

    inline constexpr char kUIFragShader[] = R"(
#version 330 core
in vec2 v_UV;
in vec4 v_Color;
uniform sampler2D u_Texture;
out vec4 FragColor;
void main() {
    FragColor = texture(u_Texture, v_UV) * v_Color;
}
)";


} // namespace UI
