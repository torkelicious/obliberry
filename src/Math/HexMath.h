#pragma once

#include "Core/Constants.h"
#include "Map/HexCoords.h"
#include <glm/glm.hpp>
#include <array>

// math
namespace Math::HexMath {
    using Core::HEX_HEIGHT_MULTIPLIER;
    using Core::HEX_HEIGHT_SPACING_RATIO;
    using Core::HEX_INV_MAT_Q_X;
    using Core::HEX_INV_MAT_Q_Y;
    using Core::HEX_INV_MAT_R_Y;
    using Core::HEX_NEIGHBOR_COUNT;
    using Core::HEX_ODD_ROW_OFFSET;
    using Core::HEX_SIZE;
    using Core::SQRT_3;

    struct FractionalHex {
        float q, r, s;

        FractionalHex(const float q_, const float r_, const float s_) : q(q_), r(r_), s(s_) {
        }
    };

    struct CubeCoords {
        int32_t x, y, z;
    };

    // translate Odd-R Offset to Cube
    inline CubeCoords OddRToCube(const Map::HexCoords hex) {
        const int32_t x = hex.q - (hex.r - (hex.r & 1)) / 2;
        const int32_t z = hex.r;
        const int32_t y = -x - z;
        return {x, y, z};
    }

    // distance calculation on hex grids
    inline int32_t Distance(const Map::HexCoords a, const Map::HexCoords b) {
        const auto [ax, ay, az] = OddRToCube(a);
        const auto [bx, by, bz] = OddRToCube(b);
        return (std::abs(ax - bx) + std::abs(ay - by) + std::abs(az - bz)) / 2;
    }


    // hex pos to world pos
    inline glm::vec2 HexToWorld(const Map::HexCoords &h, const float size = HEX_SIZE) {
        const float width = SQRT_3 * size;
        const float height = HEX_HEIGHT_MULTIPLIER * size;

        const float x = width * (static_cast<float>(h.q) + HEX_ODD_ROW_OFFSET * static_cast<float>(h.r & 1));
        const float y = height * HEX_HEIGHT_SPACING_RATIO * static_cast<float>(h.r);
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
    inline Map::HexCoords PixelToHex(const glm::vec2 p, const float size = HEX_SIZE) {
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
    inline Map::HexCoords GetClosestHex(const glm::vec2 p, const float size = HEX_SIZE) { return PixelToHex(p, size); }

    // for later evil plans ....
    inline FractionalHex Lerp(const FractionalHex &a, const FractionalHex &b, const float t) { return {a.q + (b.q - a.q) * t, a.r + (b.r - a.r) * t, a.s + (b.s - a.s) * t}; }

    inline CubeCoords CubeRound(const FractionalHex &h) {
        auto rx = static_cast<int32_t>(std::lround(h.q));
        auto ry = static_cast<int32_t>(std::lround(h.r));
        const auto rz = static_cast<int32_t>(std::lround(h.s));

        const float q_diff = std::abs(rx - h.q);
        const float r_diff = std::abs(ry - h.r);

        if (const float s_diff = std::abs(rz - h.s); q_diff > r_diff && q_diff > s_diff) {
            rx = -ry - rz;
        } else if (r_diff > s_diff) {
            ry = -rx - rz;
        }

        return {rx, ry, rz};
    }

    inline Map::HexCoords CubeToOddR(const CubeCoords &cube) {
        const int32_t col = cube.x + (cube.z - (cube.z & 1)) / 2;
        const int32_t row = cube.z;
        return {col, row};
    }

    inline FractionalHex OddRToFractionalHex(const Map::HexCoords &hex) {
        const auto [x, y, z] = OddRToCube(hex);
        return {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
    }

    inline std::vector<Map::HexCoords> GetHexLine(const Map::HexCoords &from, const Map::HexCoords &to) {
        const int32_t dist = Distance(from, to);
        if (dist == 0)
            return {};

        const auto a = OddRToFractionalHex(from);
        const auto b = OddRToFractionalHex(to);

        std::vector<Map::HexCoords> result;
        result.reserve(static_cast<size_t>(dist));

        for (int32_t i = 1; i <= dist; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(dist);
            if (const auto hex = CubeToOddR(CubeRound(Lerp(a, b, t))); result.empty() || result.back() != hex) {
                result.push_back(hex);
            }
        }

        return result;
    }

    // get 6 neighbors (for odd-r grid!!!)
    inline std::array<Map::HexCoords, HEX_NEIGHBOR_COUNT> GetNeighbors(const Map::HexCoords hex) {
        const int32_t parity = hex.r & 1;

        std::array<Map::HexCoords, HEX_NEIGHBOR_COUNT> neighbors;
        for (std::size_t i = 0; i < HEX_NEIGHBOR_COUNT; i++) {
            constexpr int32_t r_diff[HEX_NEIGHBOR_COUNT] = {0, 1, 1, 0, -1, -1};
            constexpr int32_t q_diff[2][HEX_NEIGHBOR_COUNT] = {{1, 0, -1, -1, -1, 0}, {1, 1, 0, -1, 0, 1}};
            neighbors[i] = Map::HexCoords(hex.q + q_diff[parity][i], hex.r + r_diff[i]);
        }
        return neighbors;
    }
} // namespace Math::HexMath
