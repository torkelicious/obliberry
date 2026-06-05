#include "HexMap.h"

#include <algorithm>
#include <cmath>
#include <iostream>

void HexMap::Generate(int radius) {
    tiles.clear();

    float size = HEX_SPACING;

    for (int q = -radius; q <= radius; q++) {
        for (int r = -radius; r <= radius; r++) {
            // restrict to hex shaped map 4 funsies
            int s = -q - r;
            if (std::abs(q) + std::abs(r) + std::abs(s) > radius * 2) continue;

            HexTile tile{};
            tile.q = q;
            tile.r = r;

            tile.WorldPos = HexToWorld(q, r);

            tiles.push_back(tile);
        }
    }

    std::sort(tiles.begin(), tiles.end(),
              [](const HexTile &a, const HexTile &b) { return a.WorldPos.y > b.WorldPos.y; });
}

glm::vec2 HexMap::HexToWorld(int q, int r) {
    {
        float size = HexMap::HEX_SPACING;

        return {
            size * std::sqrt(3.0f) * (q + r * 0.5f),
            size * 1.5f * r
        };
    }
}
