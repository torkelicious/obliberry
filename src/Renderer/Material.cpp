#include "Material.h"

void Material::Bind() const {
    shader->Bind();

    Texture *tex = texture ? texture : Texture::White();

    tex->Bind(0);
    shader->SetUniform1i("u_Texture", 0);

    shader->SetUniformVec4("u_Color", color);
}

void Material::Unbind() const {
    shader->Unbind();
    if (texture) {
        texture->Unbind();
    }
}
