

#ifndef OBLIBERRY_HEXCOORDS_H
#define OBLIBERRY_HEXCOORDS_H

// hex coordinates (odd-r offset, pointy-top hexes)
struct HexCoords {
    int q; // column
    int r; // row

    bool operator==(const HexCoords &other) const {
        return q == other.q && r == other.r;
    }

    bool operator<(const HexCoords &other) const {
        if (q != other.q) return q < other.q;
        return r < other.r;
    }
};

#endif //OBLIBERRY_HEXCOORDS_H
