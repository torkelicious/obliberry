#ifndef OBLIBERRY_MAPRUNTIMESYSTEM_H
#define OBLIBERRY_MAPRUNTIMESYSTEM_H

#include <optional>

#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Registry.h"
#include "ECS/Systems/MovementSystem.h"
#include "Map/HexMath.h"

namespace MapRuntimeSystem {
    inline void ResetInteractionState(MapStateComponent &state) {
        state.selectedHex = {};
        state.pathTo = {};
        state.hasSelection = false;
        state.hasPathTo = false;
    }

    inline std::optional<HexCoords> FindPreferredSpawnHex(const HexGrid &grid) {
        const HexCoords origin{0, 0};
        if (const Tile *originTile = grid.Get(origin); originTile != nullptr && originTile->walkable) {
            return origin;
        }

        for (const HexCoords &coords: grid.walkableTiles) {
            if (const Tile *tile = grid.Get(coords); tile != nullptr && tile->walkable) {
                return coords;
            }
        }

        for (const auto &[coords, tile]: grid.tiles) {
            if (tile.walkable) {
                return coords;
            }
        }

        return std::nullopt;
    }

    inline bool IsEntityPositionValidForMap(const HexGrid &grid, const TransformComponent &transform) {
        const glm::vec3 worldPosition = transform.transform.GetPosition();
        const HexCoords entityHex = HexMath::PixelToHex({worldPosition.x, worldPosition.y});
        const Tile *tile = grid.Get(entityHex);
        return tile != nullptr && tile->walkable;
    }

    inline void ResetMovementEntities(Registry &registry, const HexGrid &grid) {
        const std::optional<HexCoords> preferredSpawn = FindPreferredSpawnHex(grid);

        registry.ForEach<MovementComponent, TransformComponent>(
            [&](Entity e, MovementComponent *movement, TransformComponent *transform) {
                MovementSystem::MoveToCenter(e);
                //if (preferredSpawn.has_value() && !IsEntityPositionValidForMap(grid, *transform)) {
                //    const glm::vec2 spawnWorldPosition = HexGrid::GetWorldPos(*preferredSpawn);
                //    const glm::vec3 currentPosition = transform->transform.GetPosition();
                //    transform->transform.SetPosition({spawnWorldPosition.x, spawnWorldPosition.y, currentPosition.z});
                //}
            }
        );
    }

    inline void OnMapChanged(Registry &registry, MapComponent &map, MapStateComponent *state,
                             EngineContext &ctx) {
        ctx.renderer->Clean();
        map.needsMeshUpdate = true;

        if (state != nullptr) {
            ResetInteractionState(*state);
        }

        ResetMovementEntities(registry, map.grid);
    }

    inline void OnMapChanged(Registry &registry, EngineContext &ctx) {
        MapComponent *map = nullptr;
        MapStateComponent *state = nullptr;

        registry.ForEach<MapComponent, MapStateComponent>(
            [&](Entity, MapComponent *mapComponent, MapStateComponent *stateComponent) {
                if (map == nullptr) {
                    map = mapComponent;
                    state = stateComponent;
                }
            });

        if (map == nullptr) {
            return;
        }

        OnMapChanged(registry, *map, state, ctx);
    }
}

#endif // OBLIBERRY_MAPRUNTIMESYSTEM_H
