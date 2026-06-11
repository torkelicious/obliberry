#ifndef OBLIBERRY_MAPRENDERSYSTEM_H
#define OBLIBERRY_MAPRENDERSYSTEM_H

#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Registry.h"
#include "Renderer/MeshFactory.h"
#include "Renderer/Renderer.h"
#include "Renderer/Transform.h"

namespace MapRenderSystem {
    inline void GenerateMapTransforms(const HexGrid &grid, std::vector<glm::mat4> &grassOut,
                                      std::vector<glm::mat4> &sandOut) {
        size_t grassCount = 0;
        size_t sandCount = 0;

        for (const auto &[pos, tile]: grid.tiles) {
            if (tile.type == TileType::Grass) {
                grassCount++;
            } else if (tile.type == TileType::Sand) {
                sandCount++;
            }
        }

        grassOut.reserve(grassCount);
        sandOut.reserve(sandCount);

        for (const auto &[pos, tile]: grid.tiles) {
            const glm::vec2 worldPos = HexGrid::GetWorldPos(pos);
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(worldPos.x, worldPos.y, 0.0f));

            if (tile.type == TileType::Grass) {
                grassOut.push_back(translationMatrix);
            } else if (tile.type == TileType::Sand) {
                sandOut.push_back(translationMatrix);
            }
        }
    }

    inline void RenderTiles(Registry &registry, Renderer &renderer, EngineContext &ctx) {
        registry.ForEach<MapComponent, MapStateComponent>(
            [&](Entity, MapComponent *mapComp, MapStateComponent *stateComp) {
                if (!mapComp->hexMesh) {
                    mapComp->hexMesh = std::make_shared<Mesh>(MeshFactory::CreatePointTopHex(HEX_SIZE));
                }

                if (mapComp->needsMeshUpdate) {
                    mapComp->grassTransforms.clear();
                    mapComp->sandTransforms.clear();

                    GenerateMapTransforms(mapComp->grid, mapComp->grassTransforms, mapComp->sandTransforms);

                    mapComp->needsMeshUpdate = false;
                }

                if (!mapComp->grassTransforms.empty()) {
                    renderer.Submit(mapComp->hexMesh, mapComp->grassMat, &mapComp->grassTransforms);
                }

                if (!mapComp->sandTransforms.empty()) {
                    renderer.Submit(mapComp->hexMesh, mapComp->sandMat, &mapComp->sandTransforms);
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
