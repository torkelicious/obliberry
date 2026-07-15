#pragma once

#include "Map/HexCoords.h"

namespace ECS::Components {
    struct MapStateComponent {
        Map::HexCoords selectedHex;
        Map::HexCoords pathTo;
        bool hasSelection = false;
        bool hasPathTo = false;
        // note: both can be true at once
    };
} // namespace ECS::Components
