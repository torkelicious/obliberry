#version 330 core

in vec2 v_UV;

uniform sampler2D u_Texture;
uniform vec4 u_Color;

out vec4 FragColor;

void main()
{
    vec4 tex = texture(u_Texture, v_UV);
    FragColor = tex * u_Color;
}