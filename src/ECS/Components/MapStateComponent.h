#ifndef OBLIBERRY_MAPSTATECOMPONENT_H
#define OBLIBERRY_MAPSTATECOMPONENT_H
#include "Map/HexCoords.h"

struct MapStateComponent {
    HexCoords selectedHex;
    HexCoords pathTo;
    bool hasSelection = false;
    bool hasPathTo = false;
};

#endif //OBLIBERRY_MAPSTATECOMPONENT_H
