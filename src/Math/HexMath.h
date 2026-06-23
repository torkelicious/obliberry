#pragma once

#include "Core/Constants.h"
#include <glm/glm.hpp>

// math
namespace Math::HexMath {
    struct FractionalHex {
        float q, r, s;

        FractionalHex(const float q_, const float r_, const float s_) : q(q_), r(r_), s(s_) {
        }
    };

    struct CubeCoords {
        int32_t x, y, z;
    };

    // translate Odd-R Offset to Cube
    inline CubeCoords OddRToCube(const HexCoords hex) {
        const int32_t x = hex.q - (hex.r - (hex.r & 1)) / 2;
        const int32_t z = hex.r;
        const int32_t y = -x - z;
        return {x, y, z};
    }

    // distance calculation on hex grids
    inline int32_t Distance(const HexCoords a, const HexCoords b) {
        auto [x, y, z] = OddRToCube(a);
        const CubeCoords bc = OddRToCube(b);
        return (std::abs(x - bc.x) + std::abs(y - bc.y) + std::abs(z - bc.z)) / 2;
    }

    // hex pos to world pos
    inline glm::vec2 HexToWorld(const HexCoords &h, const float size = HEX_SIZE) {
        const float width = std::sqrt(3.0f) * size;
        const float height = HEX_HEIGHT_MULTIPLIER * size;

        float x = width * (static_cast<float>(h.q) + HEX_ODD_ROW_OFFSET * static_cast<float>(h.r & 1));
        float y = height * HEX_HEIGHT_SPACING_RATIO * static_cast<float>(h.r);
        return {x, y};
    }


    // pixel to fractional hex
    inline FractionalHex PixelToHexFractional(const glm::vec2 p, const float size = HEX_SIZE) {
        float q = (HEX_INV_MAT_Q_X * p.x + HEX_INV_MAT_Q_Y * p.y) / size;
        float r = HEX_INV_MAT_R_Y * p.y / size;
        float s = -q - r;
        return {q, r, s};
    }

    // pixel pos to hex
    inline HexCoords PixelToHex(const glm::vec2 p, const float size = HEX_SIZE) {
        const auto h = PixelToHexFractional(p, size);

        // cast to int32_t & round
        auto rq = static_cast<int32_t>(std::lround(h.q));
        auto rr = static_cast<int32_t>(std::lround(h.r));
        const auto rs = static_cast<int32_t>(std::lround(h.s));

        const float q_diff = std::abs(rq - h.q);
        const float r_diff = std::abs(rr - h.r);

        if (const float s_diff = std::abs(rs - h.s); q_diff > r_diff && q_diff > s_diff) {
            rq = -rr - rs;
        } else if (r_diff > s_diff) {
            rr = -rq - rs;
        }

        const int32_t col = rq + (rr - (rr & 1)) / 2;
        const int32_t row = rr;
        return {col, row};
    }

    // alias cuz i can :)
    inline HexCoords GetClosestHex(const glm::vec2 p, const float size = HEX_SIZE) {
        return PixelToHex(p, size);
    }

    // get 6 neighbors (for odd-r grid!!!)
    inline std::array<HexCoords, HEX_NEIGHBOR_COUNT> GetNeighbors(const HexCoords hex) {
        const int32_t parity = hex.r & 1;

        std::array<HexCoords, HEX_NEIGHBOR_COUNT> neighbors;
        for (std::size_t i = 0; i < HEX_NEIGHBOR_COUNT; i++) {
            constexpr int32_t r_diff[HEX_NEIGHBOR_COUNT] = {0, 1, 1, 0, -1, -1};
            constexpr int32_t q_diff[2][HEX_NEIGHBOR_COUNT] = {
                {1, 0, -1, -1, -1, 0},
                {1, 1, 0, -1, 0, 1}
            };
            neighbors[i] = HexCoords(
                hex.q + q_diff[parity][i],
                hex.r + r_diff[i]
            );
        }
        return neighbors;
    }
}

