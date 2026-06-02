#version 330 core

in vec2 v_UV;
out vec4 FragColor;

void main()
{
    FragColor = vec4(v_UV, 0.0, 1.0);
}