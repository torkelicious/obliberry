#pragma once

#include "Core/EngineContext.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Registry.h"
#include "Math/Frustum.h"
#include "Math/Math.h"
#include "Renderer/Renderer.h"
#include "Renderer/Transform.h"
#include <algorithm>

namespace MapRenderSystem {
[[nodiscard]] inline Math::Projection::AABB CalculateBufferedAABB(const Math::Projection::AABB& viewAABB,
                                                                  const float bufferSize = HEX_SIZE * 4.0f) noexcept {
    Math::Projection::AABB buffered = viewAABB;
    buffered.min -= glm::vec2(bufferSize);
    buffered.max += glm::vec2(bufferSize);
    return buffered;
}

struct GridBounds {
    int minQ, maxQ;
    int minR, maxR;
};

[[nodiscard]] inline GridBounds GetGridBoundsForAABB(const Math::Projection::AABB& aabb) noexcept {
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

[[nodiscard]] inline bool Contains(const Math::Projection::AABB& outer, const Math::Projection::AABB& inner) noexcept {
    return outer.min.x <= inner.min.x && outer.max.x >= inner.max.x && outer.min.y <= inner.min.y &&
           outer.max.y >= inner.max.y;
}

inline void RenderTiles(Registry& registry, Renderer& renderer, const Math::Frustum::ViewFrustum& frustum) {
    registry.ForEach<MapComponent, MapStateComponent>(
        [&](Entity, MapComponent* mapComp, const MapStateComponent* /*stateComp*/) {
            if (!mapComp->hexMesh)
                return;

            const Math::Projection::AABB cameraBounds{.min = frustum.minBounds, .max = frustum.maxBounds};

            const bool needsRebuild = mapComp->needsMeshUpdate || !Contains(mapComp->bufferedRenderAABB, cameraBounds);

            if (needsRebuild) {
                mapComp->bufferedRenderAABB = CalculateBufferedAABB(cameraBounds);
                const auto [minQ, maxQ, minR, maxR] = GetGridBoundsForAABB(mapComp->bufferedRenderAABB);

                for (auto& [typeId, transforms] : mapComp->visibles) {
                    (void)typeId;
                    transforms.clear();
                }

                for (int r = minR; r <= maxR; ++r) {
                    for (int q = minQ; q <= maxQ; ++q) {
                        if (const Tile* tile = mapComp->grid.Get(HexCoords(q, r))) {
                            mapComp->visibles[tile->type].push_back(tile->worldMatrix);
                        }
                    }
                }

                mapComp->needsMeshUpdate = false;
            }

            renderer.SetLightmap(mapComp->lightmap.texture ? &mapComp->lightmap : nullptr);
            for (auto& [typeId, transforms] : mapComp->visibles) {
                if (transforms.empty())
                    continue;

                auto matIt = mapComp->typeMats.find(typeId);
                if (matIt != mapComp->typeMats.end()) {
                    renderer.Submit(mapComp->hexMesh, &matIt->second, transforms);
                }
            }
        });
}

inline void RenderOverlays(Registry& registry, Renderer& renderer) {
    registry.ForEach<MapComponent, MapStateComponent>(
        [&](Entity, MapComponent* mapComp, const MapStateComponent* stateComp) {
            if (!mapComp->hexMesh || !mapComp->outlineMat->shader || !mapComp->pathToMat->shader) {
                return;
            }

            if (stateComp->hasSelection) {
                const glm::vec2 worldPos = HexGrid::GetWorldPos(stateComp->selectedHex);
                Transform t;
                t.SetPosition({worldPos.x, worldPos.y, 0.01f});
                t.SetScale({1.08f, 1.08f, 1.0f});
                renderer.Submit(mapComp->hexMesh, mapComp->outlineMat.get(), t);
            }

            if (stateComp->hasPathTo) {
                const glm::vec2 worldPos = HexGrid::GetWorldPos(stateComp->pathTo);
                Transform t;
                t.SetPosition({worldPos.x, worldPos.y, 0.01f});
                t.SetScale({1.08f, 1.08f, 1.0f});
                renderer.Submit(mapComp->hexMesh, mapComp->pathToMat.get(), t);
            }
        });
}

inline void RenderAll(Registry& reg, const EngineContext& ctx, const Math::Frustum::ViewFrustum& frustum) {
    RenderTiles(reg, *ctx.renderer, frustum);
    RenderOverlays(reg, *ctx.renderer);
}
} // namespace MapRenderSystem
