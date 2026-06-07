#ifndef OBLIBERRY_HEXMATH_H
#define OBLIBERRY_HEXMATH_H

// math
namespace HexMath {
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
        int x, y, z;
    };

    // translate Odd-R Offset to Cube
    inline CubeCoords OddRToCube(HexCoords hex) {
        int x = hex.q - (hex.r - (hex.r & 1)) / 2;
        int z = hex.r;
        int y = -x - z;
        return {x, y, z};
    }

    // distance calculation on hex grids
    inline int Distance(HexCoords a, HexCoords b) {
        CubeCoords ac = OddRToCube(a);
        CubeCoords bc = OddRToCube(b);
        return (std::abs(ac.x - bc.x) + std::abs(ac.y - bc.y) + std::abs(ac.z - bc.z)) / 2;
    }

    // hex pos to world pos
    inline glm::vec2 HexToWorld(const HexCoords &h, float size = HEX_SIZE) {
        float width = std::sqrt(3.0f) * size;
        float height = 2.0f * size;

        float x = width * (h.q + 0.5f * (h.r & 1));
        float y = height * 0.75f * h.r;

        return {x, y};
    }

    // pixel to fractional hex
    inline FractionalHex PixelToHexFractional(const Point &p, float size = HEX_SIZE) {
        float q = (std::sqrt(3.0f) / 3.0f * p.x - 1.0f / 3.0f * p.y) / size;
        float r = (2.0f / 3.0f * p.y) / size;
        float s = -q - r;
        return {q, r, s};
    }

    // rounding frac hex to int hex
    inline HexCoords HexRound(float q, float r, float s) {
        int rq = std::lround(q);
        int rr = std::lround(r);
        int rs = std::lround(s);

        float q_diff = std::abs(rq - q);
        float r_diff = std::abs(rr - r);
        float s_diff = std::abs(rs - s);

        if (q_diff > r_diff && q_diff > s_diff) {
            rq = -rr - rs;
        } else if (r_diff > s_diff) {
            rr = -rq - rs;
        }

        return {rq, rr};
    }

    // pixel pos to hex
    inline HexCoords PixelToHex(const Point &p, float size = HEX_SIZE) {
        auto h = PixelToHexFractional(p, size);
        HexCoords axial = HexRound(h.q, h.r, h.s);

        // from axial to odd-r offset
        int col = axial.q + (axial.r - (axial.r & 1)) / 2;
        int row = axial.r;
        return {col, row};
    }

    // alias type shi cuz i can :)
    inline HexCoords GetClosestHex(const Point &p, float size = HEX_SIZE) {
        return PixelToHex(p, size);
    }

    // get 6 neighbors (for odd-r grid!!!)
    inline std::array<HexCoords, 6> GetNeighbors(HexCoords hex) {
        int parity = hex.r & 1;
        const int q_diff[2][6] = {
            {1, 0, -1, -1, -1, 0},
            {1, 1, 0, -1, 0, 1}
        };
        const int r_diff[6] = {0, 1, 1, 0, -1, -1};

        std::array<HexCoords, 6> neighbors;
        for (int i = 0; i < 6; i++) {
            neighbors[i] = {
                hex.q + q_diff[parity][i],
                hex.r + r_diff[i]
            };
        }
        return neighbors;
    }
}

#endif //OBLIBERRY_HEXMATH_H
