#ifndef ISOMETRICGAME_MESHCOMPONENT_H
#define ISOMETRICGAME_MESHCOMPONENT_H
#include "ECS/Components.h"

// just a wrapper for now
struct MeshComponent : Component {
    Mesh *mesh = nullptr;
};

#endif //ISOMETRICGAME_MESHCOMPONENT_H
