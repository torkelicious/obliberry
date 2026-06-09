#ifndef OBLIBERRY_HEXMATH_H
#define OBLIBERRY_HEXMATH_H
#include "Core/Constants.h"

// math
namespace HexMath {
    // note: maybe depreciate this soon because its getting more in the way than not
    // i dont remember why i did this instead of a vec2 but whatever
    struct Point {
        float x;
        float y;
    };

    struct FractionalHex {
        float q, r, s;

        FractionalHex(float q_, float r_, float s_) : q(q_), r(r_), s(s_) {
        }
    };

    struct CubeCoords {
        int32_t x, y, z;
    };

    // translate Odd-R Offset to Cube
    inline CubeCoords OddRToCube(HexCoords hex) {
        int32_t x = hex.q - (hex.r - (hex.r & 1)) / 2;
        int32_t z = hex.r;
        int32_t y = -x - z;
        return {x, y, z};
    }

    // distance calculation on hex grids
    inline int32_t Distance(HexCoords a, HexCoords b) {
        CubeCoords ac = OddRToCube(a);
        CubeCoords bc = OddRToCube(b);
        return (std::abs(ac.x - bc.x) + std::abs(ac.y - bc.y) + std::abs(ac.z - bc.z)) / 2;
    }

    // hex pos to world pos
    inline glm::vec2 HexToWorld(const HexCoords &h, float size = HEX_SIZE) {
        float width = std::sqrt(3.0f) * size;
        float height = HEX_HEIGHT_MULTIPLIER * size;

        float x = width * (static_cast<float>(h.q) + HEX_ODD_ROW_OFFSET * static_cast<float>(h.r & 1));
        float y = height * HEX_HEIGHT_SPACING_RATIO * static_cast<float>(h.r);
        return {x, y};
    }


    // pixel to fractional hex
    inline FractionalHex PixelToHexFractional(const Point &p, float size = HEX_SIZE) {
        float q = (HEX_INV_MAT_Q_X * p.x + HEX_INV_MAT_Q_Y * p.y) / size;
        float r = (HEX_INV_MAT_R_Y * p.y) / size;
        float s = -q - r;
        return {q, r, s};
    }

    // pixel pos to hex
    inline HexCoords PixelToHex(const Point &p, float size = HEX_SIZE) {
        auto h = PixelToHexFractional(p, size);

        // cast to int32_t & round
        int32_t rq = static_cast<int32_t>(std::lround(h.q));
        int32_t rr = static_cast<int32_t>(std::lround(h.r));
        int32_t rs = static_cast<int32_t>(std::lround(h.s));

        float q_diff = std::abs(rq - h.q);
        float r_diff = std::abs(rr - h.r);
        float s_diff = std::abs(rs - h.s);

        if (q_diff > r_diff && q_diff > s_diff) {
            rq = -rr - rs;
        } else if (r_diff > s_diff) {
            rr = -rq - rs;
        }

        int32_t col = rq + (rr - (rr & 1)) / 2;
        int32_t row = rr;
        return HexCoords(col, row);
    }

    // alias cuz i can :)
    inline HexCoords GetClosestHex(const Point &p, float size = HEX_SIZE) {
        return PixelToHex(p, size);
    }

    // get 6 neighbors (for odd-r grid!!!)
    inline std::array<HexCoords, HEX_NEIGHBOR_COUNT> GetNeighbors(HexCoords hex) {
        int32_t parity = hex.r & 1;
        const int32_t q_diff[2][HEX_NEIGHBOR_COUNT] = {
            {1, 0, -1, -1, -1, 0},
            {1, 1, 0, -1, 0, 1}
        };
        const int32_t r_diff[HEX_NEIGHBOR_COUNT] = {0, 1, 1, 0, -1, -1};

        std::array<HexCoords, HEX_NEIGHBOR_COUNT> neighbors;
        for (std::size_t i = 0; i < HEX_NEIGHBOR_COUNT; i++) {
            neighbors[i] = HexCoords(
                hex.q + q_diff[parity][i],
                hex.r + r_diff[i]
            );
        }
        return neighbors;
    }
}

#endif //OBLIBERRY_HEXMATH_H
