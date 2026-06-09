#ifndef OBLIBERRY_DIRECTIONALTEXTURECOMPONENT_H
#define OBLIBERRY_DIRECTIONALTEXTURECOMPONENT_H
#include <array>
#include <memory>

#include "Renderer/Texture.h"

// for 6-way rotation sprite sheet stuff
struct DirectionalTextureComponent {
    std::array<std::shared_ptr<Texture>, 6> textures;
    int index = 0;
};

#endif //OBLIBERRY_DIRECTIONALTEXTURECOMPONENT_H
