#ifndef OBLIBERRY_MAPRENDERSYSTEM_H
#define OBLIBERRY_MAPRENDERSYSTEM_H

#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Registry.h"
#include "Renderer/MeshFactory.h"
#include "Renderer/Renderer.h"

namespace MapRenderSystem {
    inline MeshData CreateCombinedMapMesh(const HexGrid &grid, TileType type) {
        MeshData combined;
        const MeshData hex = MeshFactory::CreatePointTopHex(HEX_SIZE);

        // avoid repeated reallocations
        size_t tileCount = 0;
        for (const auto &[pos, tile]: grid.tiles) {
            if (tile.type == type) ++tileCount;
        }
        combined.vertices.reserve(tileCount * hex.vertices.size());
        combined.indices.reserve(tileCount * hex.indices.size());

        for (const auto &[pos, tile]: grid.tiles) {
            if (tile.type != type) continue;

            const glm::vec2 worldPos = HexGrid::GetWorldPos(pos);
            const auto vertexOffset = static_cast<uint32_t>(combined.vertices.size());

            for (auto v: hex.vertices) {
                v.Position.x += worldPos.x;
                v.Position.y += worldPos.y;
                combined.vertices.push_back(v);
            }
            for (const uint32_t index: hex.indices) {
                combined.indices.push_back(index + vertexOffset);
            }
        }
        return combined;
    }

    inline void RenderTiles(Registry &registry, Renderer &renderer) {
        registry.ForEach<MapComponent, MapStateComponent>(
            [&](Entity, MapComponent *mapComp, MapStateComponent *stateComp) {
                // rebuild combined meshes if dirty
                if (mapComp->needsMeshUpdate) {
                    MeshData grassData =
                            CreateCombinedMapMesh(mapComp->grid, TileType::Grass);
                    MeshData sandData =
                            CreateCombinedMapMesh(mapComp->grid, TileType::Sand);

                    if (!mapComp->grassMesh) {
                        mapComp->grassMesh = std::make_shared<Mesh>(grassData);
                    } else {
                        mapComp->grassMesh->Upload(grassData);
                    }

                    if (!mapComp->sandMesh) {
                        mapComp->sandMesh = std::make_shared<Mesh>(sandData);
                    } else {
                        mapComp->sandMesh->Upload(sandData);
                    }

                    mapComp->needsMeshUpdate = false;
                }

                // draw combined tiles
                Transform defaultTransform;
                if (mapComp->grassMesh && mapComp->grassMesh->GetIndexCount() > 0) {
                    renderer.Submit(*mapComp->grassMesh, mapComp->grassMat,
                                    defaultTransform);
                }
                if (mapComp->sandMesh && mapComp->sandMesh->GetIndexCount() > 0) {
                    renderer.Submit(*mapComp->sandMesh, mapComp->sandMat,
                                    defaultTransform);
                }
            });
    }

    inline void RenderOverlays(Registry &registry, Renderer &renderer) {
        registry.ForEach<MapComponent, MapStateComponent>(
            [&](Entity, MapComponent *mapComp, MapStateComponent *stateComp) {
                // mouse selection outline
                if (!mapComp->hexMesh || !mapComp->outlineMat.shader ||
                    !mapComp->pathToMat.shader) {
                    return;
                }

                if (stateComp->hasSelection) {
                    glm::vec2 worldPos =
                            mapComp->grid.GetWorldPos(stateComp->selectedHex);
                    Transform t;
                    t.SetPosition({worldPos.x, worldPos.y, 0.01f});
                    t.SetScale({1.08f, 1.08f, 1.0f});
                    renderer.Submit(*mapComp->hexMesh, mapComp->outlineMat, t);
                }

                // movement destination highlight
                if (stateComp->hasPathTo) {
                    glm::vec2 worldPos = mapComp->grid.GetWorldPos(stateComp->pathTo);
                    Transform t;
                    t.SetPosition({worldPos.x, worldPos.y, 0.01f});
                    t.SetScale({1.08f, 1.08f, 1.0f});
                    renderer.Submit(*mapComp->hexMesh, mapComp->pathToMat, t);
                }
            });
    }
}

#endif // OBLIBERRY_MAPRENDERSYSTEM_H
