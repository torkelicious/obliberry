#pragma once


#include <optional>
#include "LightingSystem.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Registry.h"
#include "ECS/Systems/MovementSystem.h"
#include "../../Math/HexMath.h"
#include "Core/EngineContext.h"

namespace ECS::Systems::MapRuntimeSystem {
    inline void ResetInteractionState(Components::MapStateComponent &state) {
        state.selectedHex = {};
        state.pathTo = {};
        state.hasSelection = false;
        state.hasPathTo = false;
    }

    inline std::optional<Map::HexCoords> FindPreferredSpawnHex(const Map::HexGrid &grid) {
        const Map::HexCoords origin{0, 0};
        if (const Map::Tile *originTile = grid.Get(origin); originTile != nullptr && originTile->walkable) {
            return origin;
        }

        for (const Map::HexCoords &coords : grid.walkableTiles) {
            if (grid.Get(coords) != nullptr) {
                return coords;
            }
        }

        for (const auto &[coords, tile] : grid.tiles) {
            if (tile.walkable) {
                return coords;
            }
        }
        return std::nullopt;
    }

    inline bool IsEntityPositionValidForMap(const Map::HexGrid &grid, const Components::TransformComponent &transform) {
        const glm::vec3 worldPosition = transform.transform.GetPosition();
        const Map::HexCoords entityHex = Math::HexMath::PixelToHex({worldPosition.x, worldPosition.y});
        const Map::Tile *tile = grid.Get(entityHex);
        return tile != nullptr && tile->walkable;
    }

    inline void ResetMovementEntities(Registry &registry, const Map::HexGrid &grid) {
        [[maybe_unused]] const std::optional<Map::HexCoords> preferredSpawn = FindPreferredSpawnHex(grid);

        registry.ForEach<Components::MovementComponent, Components::TransformComponent>(
                [&](const Entity e, Components::MovementComponent * /*movement*/,
                    Components::TransformComponent * /*transform*/) { MovementSystem::MoveToCenter(e); });
    }

    inline void OnMapChanged(Registry &registry, Components::MapComponent &map, Components::MapStateComponent *state,
                             const Core::EngineContext &ctx) {
        map.needsMeshUpdate = true;

        if (state != nullptr) {
            ResetInteractionState(*state);
        }
        // ResetMovementEntities(registry, map.grid);
        LightingSystem::GenerateLightmap(map);
    }

    inline void OnMapChanged(Registry &registry, const Core::EngineContext &ctx) {
        Components::MapComponent *map = nullptr;
        Components::MapStateComponent *state = nullptr;

        registry.ForEach<Components::MapComponent, Components::MapStateComponent>(
                [&](Entity, Components::MapComponent *mapComponent, Components::MapStateComponent *stateComponent) {
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
} // namespace ECS::Systems::MapRuntimeSystem
