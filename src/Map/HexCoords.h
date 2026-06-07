

#ifndef OBLIBERRY_HEXCOORDS_H
#define OBLIBERRY_HEXCOORDS_H

// hex coordinates (odd-r offset, pointy-top hexes)
struct HexCoords {
    int16_t q; // column
    int16_t r; // row

    // constructor casting
    HexCoords() : q(0), r(0) {
    }

    HexCoords(int32_t q_, int32_t r_) : q(static_cast<int16_t>(q_)), r(static_cast<int16_t>(r_)) {
    }

    bool operator==(const HexCoords &other) const {
        return q == other.q && r == other.r;
    }

    bool operator<(const HexCoords &other) const {
        if (q != other.q) return q < other.q;
        return r < other.r;
    }
};

// hash for 16-bit coordinates to avoid collision
struct HexCoordsHash {
    std::size_t operator()(const HexCoords &h) const noexcept {
        uint32_t packed = (static_cast<uint32_t>(static_cast<uint16_t>(h.q)) << 16) |
                          static_cast<uint16_t>(h.r);

        // MurmurHash3 finalizer scramble to spread bits evenly
        // found this somewhere online
        uint64_t x = packed;
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27;
        x *= 0x94d049bb133111ebULL;
        x ^= x >> 31;
        return static_cast<std::size_t>(x);
    }
};


#endif //OBLIBERRY_HEXCOORDS_H
