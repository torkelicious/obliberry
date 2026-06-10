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
    std::vector<HexCoords> walkableTiles;

    void Clear() {
        tiles.clear();
        walkableTiles.clear();
    }

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
        auto &tile = tiles.emplace(pos, Tile{pos, type, walkable}).first->second;
        if (walkable) {
            walkableTiles.push_back(pos);
        }
        return tile;
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

        static thread_local std::priority_queue<
            PQElement,
            std::vector<PQElement>,
            std::greater<PQElement>
        > openSet;
        // ensure queue is empty before starting
        while (!openSet.empty()) openSet.pop();

        static thread_local std::unordered_map<HexCoords, NodeRecord, HexCoordsHash> records;
        records.clear();

        static thread_local std::unordered_set<HexCoords, HexCoordsHash> closed;
        closed.clear();

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
                        P_INFINITY,
                        P_INFINITY
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
