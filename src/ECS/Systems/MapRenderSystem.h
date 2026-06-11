#ifndef OBLIBERRY_MAPRENDERSYSTEM_H
#define OBLIBERRY_MAPRENDERSYSTEM_H

#include <ranges>

#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Registry.h"
#include "Renderer/MeshFactory.h"
#include "Renderer/Renderer.h"
#include "Renderer/Transform.h"
#include "Math/Math.h"

namespace MapRenderSystem {
    inline void GenerateMapChunks(const HexGrid &grid, std::unordered_map<glm::ivec2, MapChunk> &chunks) {
        for (auto &chunk: chunks | std::views::values) {
            chunk.grassTransforms.clear();
            chunk.sandTransforms.clear();
            chunk.bounds = Math::Projection::AABB();
        }

        for (const auto &tile: grid.tiles | std::views::values) {
            const glm::vec2 &worldPos = tile.worldPos;

            glm::ivec2 chunkIdx = {
                static_cast<int>(std::floor(worldPos.x / CHUNK_SIZE)),
                static_cast<int>(std::floor(worldPos.y / CHUNK_SIZE))
            };

            auto &chunk = chunks[chunkIdx];
            chunk.bounds.Expand(worldPos);

            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(worldPos.x, worldPos.y, 0.0f));
            if (tile.type == TileType::Grass) {
                chunk.grassTransforms.push_back(translationMatrix);
            } else if (tile.type == TileType::Sand) {
                chunk.sandTransforms.push_back(translationMatrix);
            }
        }
    }

    inline void RenderTiles(Registry &registry, Renderer &renderer, EngineContext &ctx) {
        Math::Projection::AABB cameraBounds =
                Math::Projection::GetCameraGroundAABB(renderer.GetCamera(), TARGET_ASPECT);

        registry.ForEach<MapComponent, MapStateComponent>(
            [&](Entity, MapComponent *mapComp, MapStateComponent *stateComp) {
                if (!mapComp->hexMesh) {
                    mapComp->hexMesh = std::make_shared<Mesh>(MeshFactory::CreatePointTopHex(HEX_SIZE));
                }

                if (mapComp->needsMeshUpdate) {
                    GenerateMapChunks(mapComp->grid, mapComp->chunks);
                }

                bool cameraMoved = (cameraBounds.min != mapComp->lastViewBounds.min ||
                                    cameraBounds.max != mapComp->lastViewBounds.max);
                bool shouldUpdateBuffers = mapComp->needsMeshUpdate || cameraMoved;

                if (shouldUpdateBuffers) {
                    mapComp->activeGrass.clear();
                    mapComp->activeSand.clear();

                    for (auto &[idx, chunk]: mapComp->chunks) {
                        if (!cameraBounds.Intersects(chunk.bounds)) {
                            continue;
                        }

                        mapComp->activeGrass.insert(mapComp->activeGrass.end(), chunk.grassTransforms.begin(),
                                                    chunk.grassTransforms.end());
                        mapComp->activeSand.insert(mapComp->activeSand.end(), chunk.sandTransforms.begin(),
                                                   chunk.sandTransforms.end());
                    }

                    mapComp->lastViewBounds = cameraBounds;
                    mapComp->needsMeshUpdate = false;
                }

                // prevent desync
                if (!mapComp->activeGrass.empty()) {
                    renderer.Submit(mapComp->hexMesh, mapComp->grassMat, &mapComp->activeGrass, shouldUpdateBuffers);
                }

                if (!mapComp->activeSand.empty()) {
                    renderer.Submit(mapComp->hexMesh, mapComp->sandMat, &mapComp->activeSand, shouldUpdateBuffers);
                }
            });
    }

    inline void RenderOverlays(Registry &registry, Renderer &renderer) {
        registry.ForEach<MapComponent, MapStateComponent>(
            [&](Entity, MapComponent *mapComp, MapStateComponent *stateComp) {
                if (!mapComp->hexMesh ||
                    !mapComp->outlineMat->shader ||
                    !mapComp->pathToMat->shader) {
                    return;
                }

                if (stateComp->hasSelection) {
                    const glm::vec2 worldPos = HexGrid::GetWorldPos(stateComp->selectedHex);
                    Transform t;
                    t.SetPosition({worldPos.x, worldPos.y, 0.01f});
                    t.SetScale({1.08f, 1.08f, 1.0f});
                    renderer.Submit(mapComp->hexMesh, mapComp->outlineMat, t);
                }

                if (stateComp->hasPathTo) {
                    const glm::vec2 worldPos = HexGrid::GetWorldPos(stateComp->pathTo);
                    Transform t;
                    t.SetPosition({worldPos.x, worldPos.y, 0.01f});
                    t.SetScale({1.08f, 1.08f, 1.0f});
                    renderer.Submit(mapComp->hexMesh, mapComp->pathToMat, t);
                }
            });
    }

    inline void RenderAll(Registry &reg, EngineContext &ctx) {
        RenderTiles(reg, *ctx.renderer, ctx);
        RenderOverlays(reg, *ctx.renderer);
    }
}

#endif // OBLIBERRY_MAPRENDERSYSTEM_H
