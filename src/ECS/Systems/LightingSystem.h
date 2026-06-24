#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <algorithm>
#include "Core/Constants.h"
#include "Renderer/Texture.h"
#include "Renderer/Renderer.h"
#include "ECS/Registry.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/TransformComponent.h"

namespace LightingSystem {
    inline void GenerateLightmap(MapComponent &map) {
        auto &[texture, mapOffset, mapSize, ambient, accumulationBuffer, pixelBuffer] = map.lightmap;
        // world space bounding box of all tiles
        glm::vec2 minWorld(std::numeric_limits<float>::max());
        glm::vec2 maxWorld(std::numeric_limits<float>::lowest());

        for (const auto &tile: map.grid.tiles | std::views::values) {
            const glm::vec2 wp = tile.worldPos;
            minWorld = glm::min(minWorld, wp);
            maxWorld = glm::max(maxWorld, wp);
        }
        // padding
        minWorld -= glm::vec2(HEX_SIZE);
        maxWorld += glm::vec2(HEX_SIZE);

        mapOffset = minWorld;
        mapSize = maxWorld - minWorld;

        // determine texture resolution
        const int texW = std::max(1, static_cast<int>(mapSize.x / HEX_SIZE) * LIGHTMAP_TEXELS_PER_HEX);
        const int texH = std::max(1, static_cast<int>(mapSize.y / HEX_SIZE) * LIGHTMAP_TEXELS_PER_HEX);

        // resize and fill cache
        map.lightmap.accumulationBuffer.resize(texW * texH);
        map.lightmap.pixelBuffer.assign(texW * texH * 4, 255);

        if (!texture || texture->GetWidth() != texW || texture->GetHeight() != texH) {
            texture = std::make_shared<Texture>(texW, texH, map.lightmap.pixelBuffer.data());

            Renderer::SubmitInitTask([tex = texture] {
                tex->InitGL();
            });
        } else {
            Renderer::SubmitInitTask(
                [tex = texture, w = texW, h = texH, data = std::move(map.lightmap.pixelBuffer)]() mutable {
                    tex->UpdateData(data.data(), w, h);
                    data.clear();
                });
            map.lightmap.pixelBuffer.assign(texW * texH * 4, 255);
        }
    }

    inline void Update(Registry &reg) {
        MapComponent *mapComp = nullptr;
        reg.ForEach<MapComponent>([&](Entity, MapComponent *map) {
            if (!mapComp) mapComp = map;
        });
        if (!mapComp || !mapComp->lightmap.texture) return;

        auto &[texture, mapOffset, mapSize, ambient, accumulationBuffer, pixelBuffer] = mapComp->lightmap;
        const int texW = texture->GetWidth();
        const int texH = texture->GetHeight();
        const int pixelCount = texW * texH;

        if (accumulationBuffer.size() != pixelCount) {
            accumulationBuffer.resize(pixelCount);
        }
        if (pixelBuffer.size() != pixelCount * 4) {
            pixelBuffer.resize(pixelCount * 4, 255);
        }

        std::ranges::fill(accumulationBuffer, glm::vec3(ambient));

        int lightCount = 0;
        reg.ForEach<PointLightComponent, TransformComponent>(
            [&](Entity, const PointLightComponent *light, const TransformComponent *transform) {
                lightCount++;
                const glm::vec3 pos = transform->transform.GetPosition();

                const float lx = (pos.x - mapOffset.x) / mapSize.x * texW;
                const float ly = (pos.y - mapOffset.y) / mapSize.y * texH;

                const float radiusPxX = light->radius / mapSize.x * texW;
                const float radiusPxY = light->radius / mapSize.y * texH;
                const float radiusPx = std::max(radiusPxX, radiusPxY);
                const float radiusSq = radiusPx * radiusPx;
                const float invRadiusSq = 1.0f / radiusSq;

                const int minX = std::max(0, static_cast<int>(lx - radiusPx));
                const int maxX = std::min(texW - 1, static_cast<int>(lx + radiusPx));
                const int minY = std::max(0, static_cast<int>(ly - radiusPx));
                const int maxY = std::min(texH - 1, static_cast<int>(ly + radiusPx));

                for (int y = minY; y <= maxY; ++y) {
                    const float dy = static_cast<float>(y) - ly;
                    const float dySq = dy * dy;
                    const int rowIdx = y * texW;

                    for (int x = minX; x <= maxX; ++x) {
                        const float dx = static_cast<float>(x) - lx;

                        if (const float distSq = dx * dx + dySq; distSq <= radiusSq) {
                            const float falloff = std::max(0.0f, 1.0f - distSq * invRadiusSq);
                            const float brightness = falloff * light->intensity;
                            accumulationBuffer[rowIdx + x] += light->color * brightness;
                        }
                    }
                }
            });

        for (int i = 0; i < pixelCount; ++i) {
            const glm::vec3 clamped = glm::clamp(accumulationBuffer[i], 0.0f, 1.0f);
            const int pIdx = i * 4;
            pixelBuffer[pIdx + 0] = static_cast<unsigned char>(clamped.r * 255.0f);
            pixelBuffer[pIdx + 1] = static_cast<unsigned char>(clamped.g * 255.0f);
            pixelBuffer[pIdx + 2] = static_cast<unsigned char>(clamped.b * 255.0f);
            pixelBuffer[pIdx + 3] = 255;
        }
        Renderer::SubmitInitTask([tex = texture, w = texW, h = texH, data = std::move(pixelBuffer)]() mutable {
            tex->UpdateData(data.data(), w, h);
            data.clear();
        });
        pixelBuffer.assign(texW * texH * 4, 255);
    }
}
