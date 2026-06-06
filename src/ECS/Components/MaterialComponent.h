

#ifndef OBLIBERRY_MATERIALCOMPONENT_H
#define OBLIBERRY_MATERIALCOMPONENT_H
#include "Renderer/Material.h"

struct MaterialComponent : public Component {
    // material is small so im not gonna bother with using pointers
    Material material;
};

#endif //OBLIBERRY_MATERIALCOMPONENT_H
