#ifndef OBLIBERRY_HEX_H
#define OBLIBERRY_HEX_H

#include <glm/glm.hpp>
#include <unordered_map>
#include <cmath>
#include <vector>
#include <array>
#include <queue>
#include <algorithm>
#include "Core/Constants.h"

// hex coordinates (odd-r offset, pointy-top hexes)
struct HexCoords {
    int q; // column
    int r; // row

    bool operator==(const HexCoords &other) const {
        return q == other.q && r == other.r;
    }

    // sorting
    bool operator<(const HexCoords &other) const {
        if (q != other.q) return q < other.q;
        return r < other.r;
    }
};


// hash for unordered_map
struct HexCoordsHash {
    std::size_t operator()(const HexCoords &h) const noexcept {
        return std::hash<int>()(h.q) ^ (std::hash<int>()(h.r) << 1);
    }
};

// tile definitions used for texture
enum class TileType : uint8_t {
    Grass,
    Sand
};

struct Tile {
    TileType type;
    HexCoords position;
};

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

// hex grid management
class HexGrid {
public:
    using TileMap = std::unordered_map<HexCoords, Tile, HexCoordsHash>;

    TileMap tiles;

    bool HasTile(const HexCoords &pos) const {
        return tiles.find(pos) != tiles.end();
    }

    Tile *Get(const HexCoords &pos) {
        auto it = tiles.find(pos);
        return (it != tiles.end()) ? &it->second : nullptr;
    }

    const Tile *Get(const HexCoords &pos) const {
        auto it = tiles.find(pos);
        return (it != tiles.end()) ? &it->second : nullptr;
    }

    Tile &EmplaceTile(const HexCoords &pos, TileType type) {
        return tiles.emplace(pos, Tile{type, pos}).first->second;
    }

    glm::vec2 GetWorldPos(const HexCoords &pos, float size = HEX_SIZE) const {
        return HexMath::HexToWorld(pos, size);
    }

    // Performs A* Pathfinding from a start tile to a goal tile
    // Returns an ordered sequence of HexCoords from start to finish
    std::vector<HexCoords> FindPath(HexCoords start, HexCoords goal) const {
        std::vector<HexCoords> emptyPath;

        const Tile *targetTile = Get(goal);
        if (!targetTile) /* || targetTile->type == TileType::Sand) */ return emptyPath;
        if (start == goal) return {start};

        // internal tracking tracking structures
        struct NodeRecord {
            HexCoords current;
            HexCoords parent;
            int gScore = 9999999;
            int fScore = 9999999;
        };

        // priority queue sorting elements by lowest evaluated "F" Score
        using PQElement = std::pair<int, HexCoords>;
        std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement> > openSet;

        std::unordered_map<HexCoords, NodeRecord, HexCoordsHash> records;

        // initialize search starting node
        records[start] = NodeRecord{start, start, 0, HexMath::Distance(start, goal)};
        openSet.push({records[start].fScore, start});

        while (!openSet.empty()) {
            HexCoords current = openSet.top().second;
            openSet.pop();

            // destination reached
            // Backtrack step by step to reconstruct vector path
            if (current == goal) {
                std::vector<HexCoords> totalPath;
                HexCoords currTrace = goal;
                while (!(currTrace == start)) {
                    totalPath.push_back(currTrace);
                    currTrace = records[currTrace].parent;
                }
                totalPath.push_back(start);
                std::reverse(totalPath.begin(), totalPath.end());
                return totalPath;
            }

            // Loop through all neighbors of currently evaluating tile
            for (const auto &neighbor: HexMath::GetNeighbors(current)) {
                const Tile *tileData = Get(neighbor);

                // if the tile doesnt exist or its non walkable pass it (todo: implement nonwalkaable tpye
                if (!tileData) {
                    continue;
                }

                // flat weight move cost calculation
                int tentativeGScore = records[current].gScore + 1;

                if (tentativeGScore < records[neighbor].gScore) {
                    // record an optimized path tracking choice
                    records[neighbor].parent = current;
                    records[neighbor].gScore = tentativeGScore;
                    records[neighbor].fScore = tentativeGScore + HexMath::Distance(neighbor, goal);
                    openSet.push({records[neighbor].fScore, neighbor});
                }
            }
        }

        return emptyPath;
    }
};

#endif // OBLIBERRY_HEX_H
