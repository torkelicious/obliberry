#ifndef OBLIBERRY_MAPRENDERSYSTEM_H
#define OBLIBERRY_MAPRENDERSYSTEM_H

#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Registry.h"
#include "Map/HexCulling.h"
#include "Renderer/MeshFactory.h"
#include "Renderer/Renderer.h"

namespace MapRenderSystem {
    inline MeshData CreateCombinedMapMesh(
        const HexGrid &grid,
        TileType type,
        const VisibleHexRange &range) {
        MeshData combined;
        const MeshData hex = MeshFactory::CreatePointTopHex(HEX_SIZE);

        size_t tileCount = 0;

        for (const auto &[pos, tile]: grid.tiles) {
            if (tile.type != type)
                continue;

            if (pos.q < range.minQ || pos.q > range.maxQ ||
                pos.r < range.minR || pos.r > range.maxR)
                continue;

            tileCount++;
        }

        combined.vertices.reserve(tileCount * hex.vertices.size());
        combined.indices.reserve(tileCount * hex.indices.size());

        for (const auto &[pos, tile]: grid.tiles) {
            if (tile.type != type)
                continue;

            if (pos.q < range.minQ || pos.q > range.maxQ ||
                pos.r < range.minR || pos.r > range.maxR)
                continue;

            glm::vec2 worldPos = HexGrid::GetWorldPos(pos);
            uint32_t vertexOffset = static_cast<uint32_t>(combined.vertices.size());

            for (auto v: hex.vertices) {
                v.Position.x += worldPos.x;
                v.Position.y += worldPos.y;
                combined.vertices.push_back(v);
            }

            for (uint32_t idx: hex.indices) {
                combined.indices.push_back(idx + vertexOffset);
            }
        }

        return combined;
    }


    inline void RenderTiles(Registry &registry, Renderer &renderer, EngineContext &ctx) {
        registry.ForEach<MapComponent, MapStateComponent>(
            [&](Entity, MapComponent *mapComp, MapStateComponent *stateComp) {
                // render only visible
                VisibleHexRange range =
                        VisibleHexRange::Calculate(
                            *ctx.camera,
                            ctx.window->GetWidth(),
                            ctx.window->GetHeight()
                        );

                // rebuild meshes only when data changes
                if (mapComp->needsMeshUpdate) {
                    MeshData grassData =
                            CreateCombinedMapMesh(mapComp->grid, TileType::Grass, range);

                    MeshData sandData =
                            CreateCombinedMapMesh(mapComp->grid, TileType::Sand, range);

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
                    glm::vec2 worldPos =
                            mapComp->grid.GetWorldPos(stateComp->selectedHex);

                    Transform t;
                    t.SetPosition({worldPos.x, worldPos.y, 0.01f});
                    t.SetScale({1.08f, 1.08f, 1.0f});

                    renderer.Submit(mapComp->hexMesh, mapComp->outlineMat, t);
                }

                if (stateComp->hasPathTo) {
                    glm::vec2 worldPos =
                            mapComp->grid.GetWorldPos(stateComp->pathTo);

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
