#ifndef OBLIBERRY_MAPRENDERSYSTEM_H
#define OBLIBERRY_MAPRENDERSYSTEM_H

#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Registry.h"
#include "Renderer/MeshFactory.h"
#include "Renderer/Renderer.h"

namespace MapRenderSystem {
    inline void GenerateMapMeshes(const HexGrid &grid, MeshData &grassOut, MeshData &sandOut) {
        const MeshData hex = MeshFactory::CreatePointTopHex(HEX_SIZE);
        const size_t hexVertCount = hex.vertices.size();
        const size_t hexIdxCount = hex.indices.size();

        size_t grassCount = 0;
        size_t sandCount = 0;

        // combined counting pass for all tile types
        for (const auto &[pos, tile]: grid.tiles) {
            if (tile.type == TileType::Grass) {
                grassCount++;
            } else if (tile.type == TileType::Sand) {
                sandCount++;
            }
        }

        // memory allocation up front to prevent array resizing overhead
        grassOut.vertices.reserve(grassCount * hexVertCount);
        grassOut.indices.reserve(grassCount * hexIdxCount);
        sandOut.vertices.reserve(sandCount * hexVertCount);
        sandOut.indices.reserve(sandCount * hexIdxCount);

        // combined mesh building pass
        for (const auto &[pos, tile]: grid.tiles) {
            MeshData *targetMesh = nullptr;

            if (tile.type == TileType::Grass) {
                targetMesh = &grassOut;
            } else if (tile.type == TileType::Sand) {
                targetMesh = &sandOut;
            } else {
                continue;
            }

            const glm::vec2 worldPos = HexGrid::GetWorldPos(pos);
            const uint32_t vertexOffset = static_cast<uint32_t>(targetMesh->vertices.size());

            // append vertices with local world offsets
            for (const auto &v: hex.vertices) {
                auto mutatedV = v;
                mutatedV.Position.x += worldPos.x;
                mutatedV.Position.y += worldPos.y;
                targetMesh->vertices.push_back(mutatedV);
            }

            // append indices with offset shifts
            for (const uint32_t idx: hex.indices) {
                targetMesh->indices.push_back(idx + vertexOffset);
            }
        }
    }

    inline void RenderTiles(Registry &registry, Renderer &renderer, EngineContext &ctx) {
        registry.ForEach<MapComponent, MapStateComponent>(
            [&](Entity, MapComponent *mapComp, MapStateComponent *stateComp) {
                // rebuild meshes only when data changes
                if (mapComp->needsMeshUpdate) {
                    MeshData grassData;
                    MeshData sandData;

                    GenerateMapMeshes(mapComp->grid, grassData, sandData);

                    if (!mapComp->grassMesh)
                        mapComp->grassMesh = std::make_shared<Mesh>(grassData);
                    else
                        mapComp->grassMesh->Upload(grassData);

                    if (!mapComp->sandMesh)
                        mapComp->sandMesh = std::make_shared<Mesh>(sandData);
                    else
                        mapComp->sandMesh->Upload(sandData);

                    mapComp->needsMeshUpdate = false;
                }

                Transform t;

                if (mapComp->grassMesh && mapComp->grassMesh->GetIndexCount() > 0) {
                    renderer.Submit(mapComp->grassMesh, mapComp->grassMat, t);
                }

                if (mapComp->sandMesh && mapComp->sandMesh->GetIndexCount() > 0) {
                    renderer.Submit(mapComp->sandMesh, mapComp->sandMat, t);
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
                    const glm::vec2 worldPos = mapComp->grid.GetWorldPos(stateComp->selectedHex);

                    Transform t;
                    t.SetPosition({worldPos.x, worldPos.y, 0.01f});
                    t.SetScale({1.08f, 1.08f, 1.0f});

                    renderer.Submit(mapComp->hexMesh, mapComp->outlineMat, t);
                }

                if (stateComp->hasPathTo) {
                    const glm::vec2 worldPos = mapComp->grid.GetWorldPos(stateComp->pathTo);

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
