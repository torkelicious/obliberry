#pragma once

#include "Core/EngineContext.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Registry.h"
#include "Logger/LoggerService.h"
#include "Math/Frustum.h"
#include "Math/Math.h"
#include "Rendering/Renderer.h"
#include "Rendering/Transform.h"
#include <algorithm>
#include <array>
#include <unordered_set>

namespace ECS::Systems::MapRenderSystem {
    [[nodiscard]] inline Math::Projection::AABB CalculateBufferedAABB(const Math::Projection::AABB &viewAABB, const float bufferSize = Core::HEX_SIZE * 6.0f) noexcept {
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
        const Map::HexCoords tl = Math::HexMath::PixelToHex({aabb.min.x, aabb.max.y});
        const Map::HexCoords tr = Math::HexMath::PixelToHex({aabb.max.x, aabb.max.y});
        const Map::HexCoords bl = Math::HexMath::PixelToHex({aabb.min.x, aabb.min.y});
        const Map::HexCoords br = Math::HexMath::PixelToHex({aabb.max.x, aabb.min.y});

        const int minQ = std::min({tl.q, tr.q, bl.q, br.q});
        const int maxQ = std::max({tl.q, tr.q, bl.q, br.q});
        const int minR = std::min({tl.r, tr.r, bl.r, br.r});
        const int maxR = std::max({tl.r, tr.r, bl.r, br.r});

        return {minQ - 1, maxQ + 1, minR - 1, maxR + 1};
    }

    [[nodiscard]] inline bool Contains(const Math::Projection::AABB &outer, const Math::Projection::AABB &inner) noexcept {
        return outer.min.x <= inner.min.x && outer.max.x >= inner.max.x && outer.min.y <= inner.min.y && outer.max.y >= inner.max.y;
    }

    inline void RenderTiles(Registry &registry, Rendering::Renderer &renderer, const Math::Frustum::ViewFrustum &frustum) {
        registry.ForEach<Components::MapComponent, Components::MapStateComponent>([&](Entity, Components::MapComponent *mapComp, const Components::MapStateComponent * /*stateComp*/) {
            if (!mapComp->hexMesh)
                return;

            const Math::Projection::AABB cameraBounds{.min = frustum.minBounds, .max = frustum.maxBounds};

            if (const bool needsRebuild = mapComp->needsMeshUpdate || !Contains(mapComp->bufferedRenderAABB, cameraBounds)) {
                mapComp->bufferedRenderAABB = CalculateBufferedAABB(cameraBounds);
                const auto [minQ, maxQ, minR, maxR] = GetGridBoundsForAABB(mapComp->bufferedRenderAABB);

                // flip to the back buffer so the render thread can keep reading
                // the front buffer while rebuild of the geometry
                mapComp->activeBufferIndex = 1 - mapComp->activeBufferIndex;
                auto &visibleBuffer = mapComp->visibles[mapComp->activeBufferIndex];
                auto &activeTypes = mapComp->activeVisibleTypes[mapComp->activeBufferIndex];

                for (const uint8_t typeId : activeTypes) {
                    visibleBuffer[typeId].clear();
                }
                activeTypes.clear();

                std::array<size_t, 256> typeCounts{};
                for (int r = minR; r <= maxR; ++r) {
                    for (int q = minQ; q <= maxQ; ++q) {
                        if (const Map::Tile *tile = mapComp->grid.Get(Map::HexCoords(q, r))) {
                            ++typeCounts[tile->type];
                        }
                    }
                }

                for (size_t typeId = 0; typeId < typeCounts.size(); ++typeId) {
                    const size_t count = typeCounts[typeId];
                    if (count == 0) {
                        continue;
                    }
                    if (auto &transforms = visibleBuffer[typeId]; transforms.capacity() < count) {
                        transforms.reserve(count);
                    }
                    activeTypes.push_back(static_cast<uint8_t>(typeId));
                }

                for (int r = minR; r <= maxR; ++r) {
                    for (int q = minQ; q <= maxQ; ++q) {
                        if (const Map::Tile *tile = mapComp->grid.Get(Map::HexCoords(q, r))) {
                            visibleBuffer[tile->type].push_back(tile->worldMatrix);
                        }
                    }
                }

                mapComp->needsMeshUpdate = false;
            }

            renderer.SetLightmap(mapComp->lightmap.framebuffer ? &mapComp->lightmap : nullptr);
            for (const uint8_t typeId : mapComp->activeVisibleTypes[mapComp->activeBufferIndex]) {
                auto &transforms = mapComp->visibles[mapComp->activeBufferIndex][typeId];
                if (transforms.empty())
                    continue;

                auto matIt = mapComp->findTypeMat(typeId);
                if (matIt != mapComp->typeMats.end()) {
                    if (!matIt->second->texture) {
                        static std::unordered_set<uint8_t> warnedMissingTex;
                        if (warnedMissingTex.insert(typeId).second)
                            LOG_WARN("MapRender", "Tile type " + std::to_string(typeId) + " has no texture, tiles will render blank");
                    }
                    renderer.SubmitPersistent(mapComp->hexMesh, matIt->second, &transforms);
                } else {
                    static std::unordered_set<uint8_t> warnedMissingType;
                    if (warnedMissingType.insert(typeId).second)
                        LOG_WARN("MapRender", "Tile type " + std::to_string(typeId) + " has no material definition, tiles will not render");
                }
            }
        });
    }

    inline void RenderOverlays(Registry &registry, Rendering::Renderer &renderer) {
        registry.ForEach<Components::MapComponent, Components::MapStateComponent>([&](Entity, Components::MapComponent *mapComp, const Components::MapStateComponent *stateComp) {
            if (!mapComp->hexMesh || !mapComp->outlineMat || !mapComp->outlineMat->shader || !mapComp->pathToMat || !mapComp->pathToMat->shader) {
                return;
            }

            if (stateComp->hasSelection) {
                const glm::vec2 worldPos = Map::HexGrid::GetWorldPos(stateComp->selectedHex);
                Rendering::Transform t;
                t.SetPosition({worldPos.x, worldPos.y, 0.01f});
                t.SetScale({1.08f, 1.08f, 1.0f});
                renderer.Submit(mapComp->hexMesh, mapComp->outlineMat, t);
            }

            if (stateComp->hasPathTo) {
                const glm::vec2 worldPos = Map::HexGrid::GetWorldPos(stateComp->pathTo);
                Rendering::Transform t;
                t.SetPosition({worldPos.x, worldPos.y, 0.01f});
                t.SetScale({1.08f, 1.08f, 1.0f});
                renderer.Submit(mapComp->hexMesh, mapComp->pathToMat, t);
            }
        });
    }

    inline void RenderAll(Registry &reg, const Core::EngineContext &ctx, const Math::Frustum::ViewFrustum &frustum) {
        RenderTiles(reg, *ctx.renderer, frustum);
        RenderOverlays(reg, *ctx.renderer);
    }
} // namespace ECS::Systems::MapRenderSystem
