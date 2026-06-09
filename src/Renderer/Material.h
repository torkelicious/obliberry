#ifndef OBLIBERRY_MATERIAL_H
#define OBLIBERRY_MATERIAL_H
#include <memory>
#include "Shader.h"
#include "Texture.h"


struct Material {
    std::shared_ptr<Shader> shader;
    std::shared_ptr<Texture> texture;
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
};


#endif //OBLIBERRY_MATERIAL_H
