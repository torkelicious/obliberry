#version 330 core

layout(location = 0) out vec4 color;

in vec2 v_TexCoord;

uniform vec4 u_Color;
uniform sampler2D u_Texture;

void main()
{
    vec4 texColor = texture(u_Texture, v_TexCoord);
    //color = texColor;
    //color = vec4(1,1,1,1);
    color = texture(u_Texture, v_TexCoord)*u_Color;

}