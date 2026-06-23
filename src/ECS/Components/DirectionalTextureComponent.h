#pragma once

#include <array>
#include <memory>

#include "Renderer/Texture.h"

// for 6-way rotation sprite sheet stuff
struct DirectionalTextureComponent {
    std::array<std::shared_ptr<Texture>, 6> textures;
    int index = 0;
};

