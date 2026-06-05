#ifndef ISOMETRICGAME_GRIDPOSITIONCOMPONENT_H
#define ISOMETRICGAME_GRIDPOSITIONCOMPONENT_H
#include "ECS/Components.h"

struct HexPositionComponent : public Component {
    int q, r;
};
#endif //ISOMETRICGAME_GRIDPOSITIONCOMPONENT_H
