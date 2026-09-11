#pragma once

#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/Collision/CollisionSystem.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <imgui.h>

namespace Editor::ColliderGizmo {
    using Component = ECS::Components::ColliderComponent;
    using Transform = ECS::Components::TransformComponent;
    using Shape = ECS::Components::ColliderShape;
    using WorldCollider = ECS::Systems::CollisionSystem::WorldCollider;

    constexpr float handleSize = 12.0f;
    constexpr float handleHitPad = 4.0f;

    enum class HandleType : uint8_t {
        None,
        Translate,
        TL,
        TC,
        TR,
        RC,
        BR,
        BC,
        BL,
        LC,
    };

    struct Handle {
        HandleType type;
        glm::vec2 position;
    };

    inline std::array<Handle, 8> GetHandles(const WorldCollider &world) {
        const glm::vec2 half = world.shape == Shape::Box ? world.halfSize : glm::vec2(world.radius);

        const glm::vec2 lo = world.center - half;
        const glm::vec2 hi = world.center + half;
        const glm::vec2 c = world.center;

        return {{{HandleType::TL, {lo.x, hi.y}},
                 {HandleType::TC, {c.x, hi.y}},
                 {HandleType::TR, {hi.x, hi.y}},
                 {HandleType::RC, {hi.x, c.y}},
                 {HandleType::BR, {hi.x, lo.y}},
                 {HandleType::BC, {c.x, lo.y}},
                 {HandleType::BL, {lo.x, lo.y}},
                 {HandleType::LC, {lo.x, c.y}}}};
    }

    inline bool IsCorner(HandleType type) { return type == HandleType::TL || type == HandleType::TR || type == HandleType::BR || type == HandleType::BL; }

    template <typename Project> HandleType HitTest(const glm::vec2 &mouseScreen, const glm::vec2 &mouseWorld, const WorldCollider &world, Project &&toScreen) {
        constexpr float halfHit = (handleSize + handleHitPad) * 0.5f;

        // Test every handle before testing the collider interior.
        for (const auto &handle : GetHandles(world)) {
            if (world.shape == Shape::Circle && IsCorner(handle.type))
                continue;

            const ImVec2 screen = toScreen(handle.position);

            if (std::abs(mouseScreen.x - screen.x) <= halfHit && std::abs(mouseScreen.y - screen.y) <= halfHit) {
                return handle.type;
            }
        }

        const glm::vec2 delta = mouseWorld - world.center;

        if (world.shape == Shape::Circle) {
            if (glm::dot(delta, delta) <= world.radius * world.radius)
                return HandleType::Translate;
        } else {
            if (std::abs(delta.x) <= world.halfSize.x && std::abs(delta.y) <= world.halfSize.y) {
                return HandleType::Translate;
            }
        }

        return HandleType::None;
    }

    inline void TransformCollider(Component &collider, HandleType type, const glm::vec2 &delta, const Component &initial, const WorldCollider &initialWorld, const glm::vec2 &initialWorldScale) {
        const glm::vec2 scale = glm::abs(initialWorldScale);

        if (scale.x <= 0.000001f || scale.y <= 0.000001f || type == HandleType::None) {
            return;
        }

        collider = initial;

        if (type == HandleType::Translate) {
            collider.offset = initial.offset + delta / scale;
            return;
        }

        constexpr float minLocalSize = 0.001f;

        if (initial.shape == Shape::Circle) {
            float radiusDelta = 0.0f;

            switch (type) {
                case HandleType::RC:
                    radiusDelta = delta.x;
                    break;
                case HandleType::LC:
                    radiusDelta = -delta.x;
                    break;
                case HandleType::TC:
                    radiusDelta = delta.y;
                    break;
                case HandleType::BC:
                    radiusDelta = -delta.y;
                    break;
                default:
                    return;
            }

            const float radiusScale = std::max(scale.x, scale.y);

            collider.radius = std::max(minLocalSize, initial.radius + radiusDelta / radiusScale);

            return;
        }

        glm::vec2 lo = initialWorld.center - initialWorld.halfSize;
        glm::vec2 hi = initialWorld.center + initialWorld.halfSize;

        const glm::vec2 minWorldSize = scale * minLocalSize;

        const bool left = type == HandleType::TL || type == HandleType::LC || type == HandleType::BL;

        const bool right = type == HandleType::TR || type == HandleType::RC || type == HandleType::BR;

        const bool top = type == HandleType::TL || type == HandleType::TC || type == HandleType::TR;

        const bool bottom = type == HandleType::BL || type == HandleType::BC || type == HandleType::BR;

        if (left)
            lo.x = std::min(lo.x + delta.x, hi.x - minWorldSize.x);

        if (right)
            hi.x = std::max(hi.x + delta.x, lo.x + minWorldSize.x);

        if (top)
            hi.y = std::max(hi.y + delta.y, lo.y + minWorldSize.y);

        if (bottom)
            lo.y = std::min(lo.y + delta.y, hi.y - minWorldSize.y);

        const glm::vec2 newCenter = (lo + hi) * 0.5f;

        collider.size = (hi - lo) / scale;
        collider.offset = initial.offset + (newCenter - initialWorld.center) / scale;
    }

    // using imgui for this for now because i feel like it...
    template <typename Project> void DrawGizmo(ImDrawList *drawList, const WorldCollider &world, Project &&toScreen, HandleType hovered = HandleType::None) {
        if (!drawList)
            return;

        const ImU32 outline = IM_COL32(0, 204, 255, 255);
        const ImU32 border = IM_COL32(26, 26, 26, 255);
        const ImU32 normal = IM_COL32(255, 255, 255, 255);
        const ImU32 highlight = IM_COL32(77, 255, 128, 255);

        if (world.shape == Shape::Box) {
            const auto handles = GetHandles(world);

            // TL, TR, BR, BL
            constexpr int corners[] = {0, 2, 4, 6};

            for (int i = 0; i < 4; ++i) {
                drawList->AddLine(toScreen(handles[corners[i]].position), toScreen(handles[corners[(i + 1) % 4]].position), outline, 2.0f);
            }
        } else {
            constexpr int segments = 64;
            constexpr float tau = 6.28318530718f;

            for (int i = 0; i < segments; ++i) {
                const float a = tau * i / segments;
                const float b = tau * (i + 1) / segments;

                const glm::vec2 p = world.center + glm::vec2(std::cos(a), std::sin(a)) * world.radius;

                const glm::vec2 q = world.center + glm::vec2(std::cos(b), std::sin(b)) * world.radius;

                drawList->AddLine(toScreen(p), toScreen(q), outline, 2.0f);
            }
        }

        constexpr float half = handleSize * 0.5f;

        for (const auto &handle : GetHandles(world)) {
            if (world.shape == Shape::Circle && IsCorner(handle.type))
                continue;

            const ImVec2 p = toScreen(handle.position);

            drawList->AddRectFilled({p.x - half - 1.0f, p.y - half - 1.0f}, {p.x + half + 1.0f, p.y + half + 1.0f}, border);

            drawList->AddRectFilled({p.x - half, p.y - half}, {p.x + half, p.y + half}, handle.type == hovered ? highlight : normal);
        }
    }
} // namespace Editor::ColliderGizmo
