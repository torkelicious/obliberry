#pragma once


#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>
#include <unordered_set>
#include "HexCoords.h"
#include "Core/Constants.h"
#include "../Math/HexMath.h"


// tile types
// currently used for textures but i'll figure something out
// todo: not have this hardcoded, use id system maybe

using TileType = uint8_t;

struct Tile {
    // serialized fields
    HexCoords position; // 2x int16_t
    TileType type; // uint_8t
    bool walkable; // bool 1 byte
    // serialized fields
    glm::vec2 worldPos; // not serialized, only for caching!!!
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
        return it != tiles.end() ? &it->second : nullptr;
    }

    const Tile *Get(const HexCoords &pos) const {
        const auto it = tiles.find(pos);
        return it != tiles.end() ? &it->second : nullptr;
    }

    Tile &EmplaceTile(const HexCoords &pos, const TileType type, const bool walkable = true) {
        auto &tile = tiles.emplace(pos, Tile{pos, type, walkable}).first->second;
        if (walkable) {
            walkableTiles.push_back(pos);
        }
        tile.worldPos = GetWorldPos(pos);
        return tile;
    }

    static glm::vec2 GetWorldPos(const HexCoords &pos, const float size = HEX_SIZE) {
        return Math::HexMath::HexToWorld(pos, size);
    }

    // Performs A* Pathfinding from a start tile to a goal tile
    // Populates an ordered sequence of HexCoords from start to finish
    void FindPath(const HexCoords start, const HexCoords goal, std::vector<HexCoords> &outPath) const noexcept {
        outPath.clear();

        if (const Tile *targetTile = Get(goal); !targetTile || !targetTile->walkable)
            return;

        if (start == goal) {
            outPath.push_back(start);
            return;
        }

        struct NodeRecord {
            HexCoords parent;
            int gScore = P_INFINITY;
            int fScore = P_INFINITY;
            bool isClosed = false;
        };

        using PQElement = std::pair<int, HexCoords>;

        thread_local std::priority_queue<
            PQElement,
            std::vector<PQElement>,
            std::greater<>
        > openSet;
        // ensure queue is empty before starting
        openSet = decltype(openSet)();

        thread_local std::unordered_map<HexCoords, NodeRecord, HexCoordsHash> records;
        records.clear();

        // init start node
        records[start] = NodeRecord{
            start,
            0,
            Math::HexMath::Distance(start, goal),
            false
        };

        openSet.emplace(records[start].fScore, start);

        while (!openSet.empty()) {
            auto [fScoreTop, current] = openSet.top();
            openSet.pop();

            // ignore outdated entries
            if (!records.contains(current))
                continue;

            auto &currentRecord = records[current];

            if (currentRecord.isClosed)
                continue;

            if (fScoreTop != currentRecord.fScore)
                continue;

            currentRecord.isClosed = true;

            // reached goal
            if (current == goal) {
                HexCoords trace = goal;

                while (trace != start) {
                    outPath.push_back(trace);
                    trace = records[trace].parent;
                }

                outPath.push_back(start);
                std::ranges::reverse(outPath);
                return;
            }

            // Loop through all neighbors of currently evaluating tile
            for (const auto &neighbor: Math::HexMath::GetNeighbors(current)) {
                if (const Tile *tile = Get(neighbor); !tile || !tile->walkable)
                    // pass it if nonwalkable or dosent exist
                    continue;

                if (!records.contains(neighbor)) {
                    records[neighbor] = NodeRecord{
                        current,
                        P_INFINITY,
                        P_INFINITY,
                        false
                    };
                }

                auto &[parent, gScore, fScore, isClosed] = records[neighbor];
                if (isClosed)
                    continue;

                // flat weight move cost calculation

                if (const int tentativeG = currentRecord.gScore + 1; tentativeG < gScore) {
                    // record an optimized path tracking choice
                    parent = current;
                    gScore = tentativeG;
                    fScore =
                            tentativeG + Math::HexMath::Distance(neighbor, goal);

                    openSet.emplace(fScore, neighbor);
                }
            }
        }
    }
};

