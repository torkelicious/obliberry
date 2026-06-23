#pragma once

#include "Map/HexCoords.h"

struct MapStateComponent {
    HexCoords selectedHex;
    HexCoords pathTo;
    bool hasSelection = false;
    bool hasPathTo = false;
};
