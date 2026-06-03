

#ifndef ISOMETRICGAME_MATERIAL_H
#define ISOMETRICGAME_MATERIAL_H
#include <glm/vec4.hpp>
#include "Shader.h"
#include "Texture.h"


struct Material {
    Shader *shader = nullptr;
    Texture *texture = nullptr;
    glm::vec4 color = {1, 1, 1, 1};
};


#endif //ISOMETRICGAME_MATERIAL_H
