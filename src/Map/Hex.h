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


// hash for unordered_map / set
struct HexCoordsHash {
    std::size_t operator()(const HexCoords &h) const noexcept {
        return std::hash<int>()(h.q) ^ (std::hash<int>()(h.r) << 1);
    }
};

// tile types
// currently used for textures but i'll figure something out
enum class TileType : uint8_t {
    Grass,
    Sand
};

struct Tile {
    TileType type;
    HexCoords position;
    bool walkable;
};

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

    Tile &EmplaceTile(const HexCoords &pos, TileType type, bool walkable = true) {
        return tiles.emplace(pos, Tile{type, pos, walkable}).first->second;
    }

    glm::vec2 GetWorldPos(const HexCoords &pos, float size = HEX_SIZE) const {
        return HexMath::HexToWorld(pos, size);
    }

    // Performs A* Pathfinding from a start tile to a goal tile
    // Returns an ordered sequence of HexCoords from start to finish
    std::vector<HexCoords> FindPath(HexCoords start, HexCoords goal) const {
        std::vector<HexCoords> emptyPath;

        const Tile *targetTile = Get(goal);
        if (!targetTile || !targetTile->walkable)
            return emptyPath;

        if (start == goal)
            return {start};

        struct NodeRecord {
            HexCoords parent;
            int gScore = 9999999;
            int fScore = 9999999;
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
            if (records.find(current) == records.end())
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

                while (!(trace == start)) {
                    path.push_back(trace);
                    trace = records[trace].parent;
                }

                path.push_back(start);
                std::reverse(path.begin(), path.end());
                return path;
            }

            // Loop through all neighbors of currently evaluating tile
            for (const auto &neighbor: HexMath::GetNeighbors(current)) {
                const Tile *tile = Get(neighbor);
                if (!tile || !tile->walkable) // pass it if nonwalkable or dosent exist
                    continue;

                if (!records.contains(neighbor)) {
                    records[neighbor] = NodeRecord{
                        current,
                        9999999,
                        9999999
                    };
                }

                // flat weight move cost calculation
                int tentativeG = records[current].gScore + 1;

                if (tentativeG < records[neighbor].gScore) {
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
