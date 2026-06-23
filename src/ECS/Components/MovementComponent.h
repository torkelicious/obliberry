#pragma once

#include <vector>
#include "Map/HexCoords.h"


struct MovementComponent {
    std::vector<HexCoords> currentPath{};
    size_t currentPathIndex = 0;
    float stepTimer = 0.0f;
    float timePerStep = 0.15f;
    float idleTimer = 0.0f;
    bool isMoving = false;
};

