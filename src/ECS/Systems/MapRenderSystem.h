#ifndef OBLIBERRY_MAPRENDERSYSTEM_H
#define OBLIBERRY_MAPRENDERSYSTEM_H

#include <algorithm>
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Registry.h"
#include "Renderer/MeshFactory.h"
#include "Renderer/Renderer.h"
#include "Renderer/Transform.h"
#include "Math/Math.h"

namespace MapRenderSystem {
    [[nodiscard]] inline bool Contains(const Math::Projection::AABB &outer,
                                       const Math::Projection::AABB &inner) noexcept {
        return outer.min.x <= inner.min.x && outer.max.x >= inner.max.x &&
               outer.min.y <= inner.min.y && outer.max.y >= inner.max.y;
    }

    [[nodiscard]] inline Math::Projection::AABB CalculateBufferedAABB(const Math::Projection::AABB &viewAABB,
                                                                      const float bufferSize = HEX_SIZE * 4.0f)
        noexcept {
        Math::Projection::AABB buffered = viewAABB;
        buffered.min -= glm::vec2(bufferSize);
        buffered.max += glm::vec2(bufferSize);
        return buffered;
    }

    struct GridBounds {
        int minQ, maxQ;
        int minR, maxR;
    };

    [[nodiscard]] inline GridBounds GetGridBoundsForAABB(const Math::Projection::AABB &aabb) noexcept {
        const HexCoords tl = Math::HexMath::PixelToHex({aabb.min.x, aabb.max.y});
        const HexCoords tr = Math::HexMath::PixelToHex({aabb.max.x, aabb.max.y});
        const HexCoords bl = Math::HexMath::PixelToHex({aabb.min.x, aabb.min.y});
        const HexCoords br = Math::HexMath::PixelToHex({aabb.max.x, aabb.min.y});

        const int minQ = std::min({tl.q, tr.q, bl.q, br.q});
        const int maxQ = std::max({tl.q, tr.q, bl.q, br.q});
        const int minR = std::min({tl.r, tr.r, bl.r, br.r});
        const int maxR = std::max({tl.r, tr.r, bl.r, br.r});

        return {minQ - 1, maxQ + 1, minR - 1, maxR + 1};
    }

    inline void RenderTiles(Registry &registry, Renderer &renderer) {
        const Math::Projection::AABB cameraBounds =
                Math::Projection::GetCameraGroundAABB(renderer.GetCamera(), TARGET_ASPECT);

        registry.ForEach<MapComponent, MapStateComponent>(
            [&](Entity, MapComponent *mapComp, const MapStateComponent */*stateComp*/) {
                if (!mapComp->hexMesh) {
                    mapComp->hexMesh = std::make_shared<Mesh>(MeshFactory::CreatePointTopHex(HEX_SIZE));
                }

                const bool isContained = Contains(mapComp->bufferedRenderAABB, cameraBounds);
                const bool shouldUpdateBuffers = mapComp->needsMeshUpdate || !isContained;

                if (shouldUpdateBuffers) {
                    mapComp->bufferedRenderAABB = CalculateBufferedAABB(cameraBounds);
                    for (auto &transforms: mapComp->visibles | std::views::values) {
                        transforms.clear();
                    }

                    const auto [minQ, maxQ, minR, maxR] = GetGridBoundsForAABB(mapComp->bufferedRenderAABB);

                    for (int r = minR; r <= maxR; ++r) {
                        for (int q = minQ; q <= maxQ; ++q) {
                            if (const Tile *tile = mapComp->grid.Get(HexCoords(q, r))) {
                                const glm::mat4 translationMatrix = glm::translate(
                                    glm::mat4(1.0f), glm::vec3(tile->worldPos.x, tile->worldPos.y, 0.0f));

                                // push the matrix instance into the vector for this specific tile type
                                mapComp->visibles[tile->type].push_back(translationMatrix);
                            }
                        }
                    }

                    mapComp->needsMeshUpdate = false;
                }

                renderer.SetLightmap(mapComp->lightmap.texture ? &mapComp->lightmap : nullptr);
                // loop through all collected tile types and submit their instanced buffers
                for (auto &[typeId, transforms]: mapComp->visibles) {
                    if (transforms.empty()) continue;

                    auto matIt = mapComp->typeMats.find(typeId);
                    if (matIt != mapComp->typeMats.end()) {
                        renderer.Submit(mapComp->hexMesh.get(), &matIt->second, &transforms, shouldUpdateBuffers);
                    }
                }
            });
    }

    inline void RenderOverlays(Registry &registry, Renderer &renderer) {
        registry.ForEach<MapComponent, MapStateComponent>(
            [&](Entity, MapComponent *mapComp, const MapStateComponent *stateComp) {
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
                    renderer.Submit(mapComp->hexMesh.get(), mapComp->outlineMat.get(), t);
                }

                if (stateComp->hasPathTo) {
                    const glm::vec2 worldPos = HexGrid::GetWorldPos(stateComp->pathTo);
                    Transform t;
                    t.SetPosition({worldPos.x, worldPos.y, 0.01f});
                    t.SetScale({1.08f, 1.08f, 1.0f});
                    renderer.Submit(mapComp->hexMesh.get(), mapComp->pathToMat.get(), t);
                }
            });
    }

    inline void RenderAll(Registry &reg, EngineContext &ctx) {
        RenderTiles(reg, *ctx.renderer);
        RenderOverlays(reg, *ctx.renderer);
    }
}

#endif // OBLIBERRY_MAPRENDERSYSTEM_H
