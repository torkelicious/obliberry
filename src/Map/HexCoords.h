#pragma once
#include <cstdint>

namespace Map {
    // hex coordinates (odd-r offset, pointy-top hexes)
    struct HexCoords {
        int16_t q; // column
        int16_t r; // row

        // constructor casting
        HexCoords() : q(0), r(0) {}

        HexCoords(const int32_t q_, const int32_t r_) : q(static_cast<int16_t>(q_)), r(static_cast<int16_t>(r_)) {}

        bool operator==(const HexCoords &other) const { return q == other.q && r == other.r; }

        bool operator<(const HexCoords &other) const {
            if (q != other.q)
                return q < other.q;
            return r < other.r;
        }
    };

    // identity hash
    struct HexCoordsHash {
        std::size_t operator()(const HexCoords &h) const noexcept {
            return static_cast<uint32_t>(static_cast<uint16_t>(h.q)) << 16 |
                   static_cast<uint32_t>(static_cast<uint16_t>(h.r));
        }
    };
} // namespace Map
