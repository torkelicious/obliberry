

#ifndef OBLIBERRY_MATERIAL_H
#define OBLIBERRY_MATERIAL_H
#include "Shader.h"
#include "Texture.h"


class Material {
public:
    //Material();
    //~Material();
    Shader *shader;
    Texture *texture;
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};

    void Bind() const;

    void Unbind() const;
};


#endif //OBLIBERRY_MATERIAL_H
