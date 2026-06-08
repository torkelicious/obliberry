#ifndef OBLIBERRY_MATERIALCOMPONENT_H
#define OBLIBERRY_MATERIALCOMPONENT_H
#include "Renderer/Material.h"

// material is small so im not gonna bother with using pointers
struct MaterialComponent {
    Material material{};
};

#endif //OBLIBERRY_MATERIALCOMPONENT_H
