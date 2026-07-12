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

namespace Map {
    using TileType = uint8_t;

    struct Tile {
        glm::mat4 worldMatrix{1.0f};
        glm::vec2 worldPos;
        HexCoords position;
        TileType type;
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

        bool HasTile(const HexCoords &pos) const { return tiles.contains(pos); }

        Tile *Get(const HexCoords &pos) {
            const auto it = tiles.find(pos);
            return it != tiles.end() ? &it->second : nullptr;
        }

        const Tile *Get(const HexCoords &pos) const {
            const auto it = tiles.find(pos);
            return it != tiles.end() ? &it->second : nullptr;
        }

        Tile &EmplaceTile(const HexCoords &pos, const TileType type, const bool walkable = true) {
            auto &tile = tiles.emplace(pos, Tile{.position = pos, .type = type, .walkable = walkable}).first->second;
            if (walkable) {
                walkableTiles.push_back(pos);
            }
            tile.worldPos = GetWorldPos(pos);
            tile.worldMatrix[3] = glm::vec4(tile.worldPos, 0.0f, 1.0f);
            return tile;
        }

        bool RemoveTileAt(const HexCoords &pos) {
            const bool removed = tiles.erase(pos) > 0;
            if (removed) {
                std::erase(walkableTiles, pos);
            }
            return removed;
        }

        void SyncTileWalkableCache(const HexCoords &pos) {
            if (const auto *tile = Get(pos)) {
                if (tile->walkable) {
                    if (std::ranges::find(walkableTiles, pos) == walkableTiles.end()) {
                        walkableTiles.push_back(pos);
                    }
                } else {
                    std::erase(walkableTiles, pos);
                }
            }
        }

        static glm::vec2 GetWorldPos(const HexCoords &pos, const float size = Core::HEX_SIZE) { return Math::HexMath::HexToWorld(pos, size); }

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
                int gScore = Core::P_INFINITY;
                int fScore = Core::P_INFINITY;
                bool isClosed = false;
            };

            using PQElement = std::pair<int, HexCoords>;

            thread_local std::priority_queue<PQElement, std::vector<PQElement>, std::greater<>> openSet;
            // ensure queue is empty before starting
            openSet = decltype(openSet)();

            thread_local std::unordered_map<HexCoords, NodeRecord, HexCoordsHash> records;
            records.clear();

            // init start node
            auto [startIt, startInserted] = records.try_emplace(start, NodeRecord{start, 0, Math::HexMath::Distance(start, goal), false});
            openSet.emplace(startIt->second.fScore, start);

            while (!openSet.empty()) {
                auto [fScoreTop, current] = openSet.top();
                openSet.pop();

                auto currentIt = records.find(current);
                if (currentIt == records.end())
                    continue;

                auto &currentRecord = currentIt->second;

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
                for (const auto &neighbor : Math::HexMath::GetNeighbors(current)) {
                    if (const Tile *tile = Get(neighbor); !tile || !tile->walkable)
                        continue;

                    auto [nbIt, inserted] = records.try_emplace(neighbor, NodeRecord{current, Core::P_INFINITY, Core::P_INFINITY, false});

                    auto &[parent, gScore, fScore, isClosed] = nbIt->second;
                    if (isClosed)
                        continue;

                    // flat weight move cost calculation
                    if (const int tentativeG = currentRecord.gScore + 1; tentativeG < gScore) {
                        parent = current;
                        gScore = tentativeG;
                        fScore = tentativeG + Math::HexMath::Distance(neighbor, goal);
                        openSet.emplace(fScore, neighbor);
                    }
                }
            }
        }
    };
} // namespace Map
