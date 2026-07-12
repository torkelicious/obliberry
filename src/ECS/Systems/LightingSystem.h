#pragma once

#include "Core/Constants.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Registry.h"
#include "Rendering/Renderer.h"
#include "Rendering/Texture.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <memory>

namespace ECS::Systems::LightingSystem {
    inline void GenerateLightmap(Components::MapComponent &map) {
        auto &lm = map.lightmap;

        // world space bounding box of all tiles
        glm::vec2 minWorld(std::numeric_limits<float>::max());
        glm::vec2 maxWorld(std::numeric_limits<float>::lowest());

        for (const auto &tile : map.grid.tiles | std::views::values) {
            const glm::vec2 wp = tile.worldPos;
            minWorld = glm::min(minWorld, wp);
            maxWorld = glm::max(maxWorld, wp);
        }
        // padding
        minWorld -= glm::vec2(Core::HEX_SIZE);
        maxWorld += glm::vec2(Core::HEX_SIZE);

        lm.mapOffset = minWorld;
        lm.mapSize = maxWorld - minWorld;

        // determine texture resolution
        const int texW = std::max(1, static_cast<int>(lm.mapSize.x / Core::HEX_SIZE) * Core::LIGHTMAP_TEXELS_PER_HEX);
        const int texH = std::max(1, static_cast<int>(lm.mapSize.y / Core::HEX_SIZE) * Core::LIGHTMAP_TEXELS_PER_HEX);

        // resize and fill cache
        lm.accumulationBuffer.resize(texW * texH);
        lm.pixelBuffer.assign(texW * texH * 4, 255);

        if (!lm.texture || lm.texture->GetWidth() != texW || lm.texture->GetHeight() != texH) {
            lm.texture = std::make_shared<Rendering::Texture>(texW, texH, lm.pixelBuffer.data());

            Rendering::Renderer::SubmitInitTask([tex = lm.texture] { tex->InitGL(); });
        } else {
            Rendering::Renderer::SubmitInitTask([tex = lm.texture, w = texW, h = texH, data = std::move(lm.pixelBuffer)]() mutable {
                tex->UpdateData(data.data(), w, h);
                data.clear();
            });
            lm.pixelBuffer.assign(texW * texH * 4, 255);
        }

        // force a rebuild the first time
        lm.lastLightCount = std::numeric_limits<size_t>::max();
    }

    // return true if anything relevant to the lightmap changed since the last rebuild.
    inline bool ConsumeDirtyState(Registry &reg, const Components::MapComponent &mapComp, const size_t lightCount) {
        bool anyDirty = (lightCount != mapComp.lightmap.lastLightCount);

        reg.ForEach<Components::PointLightComponent, Components::TransformComponent>([&](Entity, const Components::PointLightComponent *light, const Components::TransformComponent *transform) {
            if (light->dirty || transform->transform.IsDirty())
                anyDirty = true;
        });

        return anyDirty;
    }

    inline void Update(Registry &reg) {
        Components::MapComponent *mapComp = reg.GetFirst<Components::MapComponent>();
        if (!mapComp || !mapComp->lightmap.texture) {
            return;
        }

        auto *lightPool = reg.GetPool<Components::PointLightComponent>();
        const size_t lightCount = lightPool->GetDenseEntities().size();
        if (lightCount == 0)
            return;

        if (!ConsumeDirtyState(reg, *mapComp, lightCount))
            return;

        auto &[accumulationBuffer, pixelBuffer, texture, mapOffset, mapSize, ambient, lastLightCount] = mapComp->lightmap;
        const int texW = texture->GetWidth();
        const int texH = texture->GetHeight();
        const int pixelCount = texW * texH;

        if (accumulationBuffer.size() != static_cast<size_t>(pixelCount)) {
            accumulationBuffer.resize(pixelCount);
        }
        if (pixelBuffer.size() != static_cast<size_t>(pixelCount * 4)) {
            pixelBuffer.resize(pixelCount * 4, 255);
        }

        std::ranges::fill(accumulationBuffer, glm::vec3(ambient));

        reg.ForEach<Components::PointLightComponent, Components::TransformComponent>([&](Entity, const Components::PointLightComponent *light, const Components::TransformComponent *transform) {
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
        Rendering::Renderer::SubmitInitTask([tex = texture, w = texW, h = texH, data = std::move(pixelBuffer)]() mutable {
            tex->UpdateData(data.data(), w, h);
            data.clear();
        });
        pixelBuffer.assign(texW * texH * 4, 255);

        // rebuild complete
        lastLightCount = lightCount;
        reg.ForEach<Components::PointLightComponent, Components::TransformComponent>([&](Entity, Components::PointLightComponent *light, const Components::TransformComponent *transform) {
            light->dirty = false;
            transform->transform.ClearDirty();
        });
    }
} // namespace ECS::Systems::LightingSystem
