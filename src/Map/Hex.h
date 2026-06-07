#ifndef OBLIBERRY_HEX_H
#define OBLIBERRY_HEX_H

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>
#include <unordered_set>
#include "HexCoords.h"
#include "Core/Constants.h"
#include "HexMath.h"


// migrated to hexcoords murmurhash3
// hash for unordered_map / set
//struct HexCoordsHash {
//    std::size_t operator()(const HexCoords &h) const noexcept {
//        return std::hash<int>()(h.q) ^ (std::hash<int>()(h.r) << 1);
//    }
//};

// tile types
// currently used for textures but i'll figure something out
enum class TileType : uint8_t {
    Grass,
    Sand
};

struct Tile {
    HexCoords position; // 2x int16_t
    TileType type; // uint_8t
    bool walkable;
};

// hex grid management
class HexGrid {
public:
    using TileMap = std::unordered_map<HexCoords, Tile, HexCoordsHash>;

    TileMap tiles;

    bool HasTile(const HexCoords &pos) const {
        return tiles.contains(pos);
    }

    Tile *Get(const HexCoords &pos) {
        const auto it = tiles.find(pos);
        return (it != tiles.end()) ? &it->second : nullptr;
    }

    const Tile *Get(const HexCoords &pos) const {
        const auto it = tiles.find(pos);
        return (it != tiles.end()) ? &it->second : nullptr;
    }

    Tile &EmplaceTile(const HexCoords &pos, TileType type, bool walkable = true) {
        return tiles.emplace(pos, Tile{pos, type, walkable}).first->second;
    }

    static glm::vec2 GetWorldPos(const HexCoords &pos, const float size = HEX_SIZE) {
        return HexMath::HexToWorld(pos, size);
    }

    // Performs A* Pathfinding from a start tile to a goal tile
    // Returns an ordered sequence of HexCoords from start to finish
    std::vector<HexCoords> FindPath(HexCoords start, const HexCoords goal) const {
        std::vector<HexCoords> emptyPath;

        if (const Tile *targetTile = Get(goal); !targetTile || !targetTile->walkable)
            return emptyPath;

        if (start == goal)
            return {start};

        struct NodeRecord {
            HexCoords parent;
            int gScore = P_INFINITY;
            int fScore = P_INFINITY;
        };

        using PQElement = std::pair<int, HexCoords>;

        std::priority_queue<
            PQElement,
            std::vector<PQElement>,
            std::greater<PQElement>
        > openSet;

        std::unordered_map<HexCoords, NodeRecord, HexCoordsHash> records;
        std::unordered_set<HexCoords, HexCoordsHash> closed;

        // init start node
        records[start] = NodeRecord{
            start,
            0,
            HexMath::Distance(start, goal)
        };

        openSet.push({records[start].fScore, start});

        while (!openSet.empty()) {
            auto [fScoreTop, current] = openSet.top();
            openSet.pop();

            // ignore outdated entries
            if (!records.contains(current))
                continue;

            if (fScoreTop != records[current].fScore)
                continue;

            if (closed.contains(current))
                continue;

            closed.insert(current);

            // reached goal
            if (current == goal) {
                std::vector<HexCoords> path;
                HexCoords trace = goal;

                while (trace != start) {
                    path.push_back(trace);
                    trace = records[trace].parent;
                }

                path.push_back(start);
                std::ranges::reverse(path);
                return path;
            }

            // Loop through all neighbors of currently evaluating tile
            for (const auto &neighbor: HexMath::GetNeighbors(current)) {
                if (const Tile *tile = Get(neighbor); !tile || !tile->walkable)
                    // pass it if nonwalkable or dosent exist
                    continue;

                if (!records.contains(neighbor)) {
                    records[neighbor] = NodeRecord{
                        current,
                        9999999,
                        9999999
                    };
                }

                // flat weight move cost calculation

                if (const int tentativeG = records[current].gScore + 1; tentativeG < records[neighbor].gScore) {
                    // record an optimized path tracking choice
                    records[neighbor].parent = current;
                    records[neighbor].gScore = tentativeG;
                    records[neighbor].fScore =
                            tentativeG + HexMath::Distance(neighbor, goal);

                    openSet.push({records[neighbor].fScore, neighbor});
                }
            }
        }
        return emptyPath;
    }
};

#endif // OBLIBERRY_HEX_H
