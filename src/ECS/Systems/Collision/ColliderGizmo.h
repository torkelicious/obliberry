#pragma once

#include "ECS/Systems/Collision/ColliderGeometry.h"

#include <imgui.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace Editor::ColliderGizmo {
    using WorldCollider = ECS::Collision::WorldCollider;
    using Shape = ECS::Components::ColliderShape;

    enum class HandleType : uint8_t { None, Translate, Face, Radius, Height };

    struct Handle {
        HandleType type = HandleType::None;
        glm::dvec3 localPosition{0.0};
        glm::dvec3 direction{0.0};
    };

    inline glm::dvec3 ToWorldPoint(const WorldCollider &world, const glm::dvec3 &local) { return glm::dvec3(world.localToWorld * glm::dvec4(local, 1.0)); }

    inline glm::dmat4 MakeMatrix(const glm::mat4 &viewProjection, const WorldCollider &world) { return glm::dmat4(viewProjection) * world.localToWorld; }

    inline bool ToScreen(const glm::dmat4 &matrix, const glm::dvec3 &local, ImVec2 viewportMin, ImVec2 viewportMax, glm::vec2 &outScreen) {
        const glm::dvec4 clip = matrix * glm::dvec4(local, 1.0);

        if (clip.w <= 0.0)
            return false;

        const glm::dvec3 ndc = glm::dvec3(clip) / clip.w;

        const double width = viewportMax.x - viewportMin.x;
        const double height = viewportMax.y - viewportMin.y;

        outScreen = {float(viewportMin.x + (ndc.x * 0.5 + 0.5) * width), float(viewportMin.y + (0.5 - ndc.y * 0.5) * height)};
        return true;
    }

    inline glm::dvec3 WorldDeltaToLocal(const WorldCollider &world, const glm::dvec3 &deltaWorld) {
        glm::dvec3 result(0.0);

        for (int i = 0; i < 3; ++i) {
            const glm::dvec3 axis = glm::dvec3(world.localToWorld[i]);
            const double length = glm::length(axis);

            if (length > 1e-12) {
                result[i] = glm::dot(axis / length, deltaWorld) / length;
            }
        }

        return result;
    }

    inline double AxisFacing(const WorldCollider &world, const Handle &handle, const glm::dvec3 &viewDirection) {
        if (handle.type == HandleType::Translate)
            return 0.0;

        const glm::dvec3 axisWorld = glm::dvec3(world.localToWorld * glm::dvec4(handle.direction, 0.0));
        const double length = glm::length(axisWorld);

        if (length <= 1e-12)
            return 1.0;

        return std::abs(glm::dot(axisWorld / length, viewDirection));
    }

    inline std::vector<Handle> GetHandles(const WorldCollider &world) {
        std::vector<Handle> handles;

        auto push = [&](HandleType type, const glm::dvec3 &local, const glm::dvec3 &direction) { handles.push_back({type, local, direction}); };

        // translate handle at the collider center
        push(HandleType::Translate, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0});

        const auto &c = world.geometry;

        if (c.shape == Shape::Box) {
            push(HandleType::Face, {c.size.x * 0.5f, 0.0f, 0.0f}, {1.0, 0.0, 0.0});
            push(HandleType::Face, {-c.size.x * 0.5f, 0.0f, 0.0f}, {-1.0, 0.0, 0.0});
            push(HandleType::Face, {0.0f, c.size.y * 0.5f, 0.0f}, {0.0, 1.0, 0.0});
            push(HandleType::Face, {0.0f, -c.size.y * 0.5f, 0.0f}, {0.0, -1.0, 0.0});
            push(HandleType::Face, {0.0f, 0.0f, c.size.z * 0.5f}, {0.0, 0.0, 1.0});
            push(HandleType::Face, {0.0f, 0.0f, -c.size.z * 0.5f}, {0.0, 0.0, -1.0});
        } else if (c.shape == Shape::Sphere) {
            push(HandleType::Radius, {c.radius, 0.0f, 0.0f}, {1.0, 0.0, 0.0});
            push(HandleType::Radius, {-c.radius, 0.0f, 0.0f}, {-1.0, 0.0, 0.0});
            push(HandleType::Radius, {0.0f, c.radius, 0.0f}, {0.0, 1.0, 0.0});
            push(HandleType::Radius, {0.0f, -c.radius, 0.0f}, {0.0, -1.0, 0.0});
            push(HandleType::Radius, {0.0f, 0.0f, c.radius}, {0.0, 0.0, 1.0});
            push(HandleType::Radius, {0.0f, 0.0f, -c.radius}, {0.0, 0.0, -1.0});
        } else {
            const double radius = c.radius;
            const double half = c.height * 0.5f;

            push(HandleType::Radius, {radius, 0.0f, 0.0f}, {1.0, 0.0, 0.0});
            push(HandleType::Radius, {-radius, 0.0f, 0.0f}, {-1.0, 0.0, 0.0});
            push(HandleType::Radius, {0.0f, 0.0f, radius}, {0.0, 0.0, 1.0});
            push(HandleType::Radius, {0.0f, 0.0f, -radius}, {0.0, 0.0, -1.0});
            push(HandleType::Height, {0.0f, half, 0.0f}, {0.0, 1.0, 0.0});
            push(HandleType::Height, {0.0f, -half, 0.0f}, {0.0, -1.0, 0.0});
        }

        return handles;
    }

    inline int HitTest(const glm::vec2 &mouseScreen, const WorldCollider &world, const glm::mat4 &viewProjection, const glm::dvec3 &viewDirection, ImVec2 viewportMin, ImVec2 viewportMax) {
        constexpr float hitRadius = 12.0f;
        constexpr double unusable = 0.95;

        const glm::dmat4 matrix = MakeMatrix(viewProjection, world);
        const std::vector<Handle> handles = GetHandles(world);

        int best = -1;
        double bestDistSqred = hitRadius * hitRadius;

        for (size_t i = 0; i < handles.size(); ++i) {
            if (AxisFacing(world, handles[i], viewDirection) > unusable)
                continue;

            glm::vec2 screen{0.0f, 0.0f};

            if (!ToScreen(matrix, handles[i].localPosition, viewportMin, viewportMax, screen))
                continue;

            const glm::vec2 delta = mouseScreen - screen;
            const double distSqred = glm::dot(glm::dvec2(delta), glm::dvec2(delta));

            if (distSqred <= bestDistSqred) {
                bestDistSqred = distSqred;
                best = static_cast<int>(i);
            }
        }

        return best;
    }

    inline void TransformCollider(ECS::Components::ColliderComponent &collider, const Handle &handle, const glm::dvec3 &localDelta) {
        constexpr double minimum = 0.001;
        const auto &d = handle.direction;

        switch (handle.type) {
            case HandleType::Translate: {
                collider.offset += glm::vec3(localDelta);
                return;
            }
            case HandleType::Radius: {
                collider.radius = float(std::max(minimum, double(collider.radius) + glm::dot(d, localDelta)));
                return;
            }
            case HandleType::Height: {
                // move one cap while the opposite cap stays anchored
                const double half = collider.height * 0.5;
                const double anchor = -d.y * half;
                const double cap = d.y * half + glm::dot(d, localDelta);
                const double newHalf = std::max(minimum, (cap - anchor) * d.y);
                const double newCap = anchor + d.y * newHalf;

                collider.height = float(newHalf * 2.0);
                collider.offset.y = float((anchor + newCap) * 0.5);
                return;
            }
            case HandleType::Face: {
                // move one face while the opposite face stays anchored
                const int axis = d.x != 0.0 ? 0 : (d.y != 0.0 ? 1 : 2);
                const double s = d[axis];
                const double size = collider.size[axis];
                const double anchor = collider.offset[axis] - s * size * 0.5;
                const double face = collider.offset[axis] + s * size * 0.5 + glm::dot(d, localDelta);
                const double newSize = std::max(minimum, (face - anchor) * s);
                const double newFace = anchor + s * newSize;

                collider.size[axis] = float(newSize);
                collider.offset[axis] = float((anchor + newFace) * 0.5);
                return;
            }
            case HandleType::None:
                return;
        }
    }

    inline void Draw(ImDrawList *drawList, const WorldCollider &world, const glm::mat4 &viewProjection, const glm::dvec3 &viewDirection, ImVec2 viewportMin, ImVec2 viewportMax, int hovered = -1) {
        if (!drawList)
            return;

        const double width = viewportMax.x - viewportMin.x;
        const double height = viewportMax.y - viewportMin.y;

        if (width <= 0.0 || height <= 0.0)
            return;

        constexpr double unusable = 0.95;

        const glm::dmat4 matrix = MakeMatrix(viewProjection, world);

        drawList->PushClipRect(viewportMin, viewportMax, true);

        auto line = [&](glm::dvec3 a, glm::dvec3 b) {
            glm::dvec4 ca = matrix * glm::dvec4(a, 1.0);
            glm::dvec4 cb = matrix * glm::dvec4(b, 1.0);

            for (double sign : {-1.0, 1.0}) {
                const double da = ca.w + sign * ca.z;
                const double db = cb.w + sign * cb.z;

                if (da < 0.0 && db < 0.0)
                    return;

                if (da < 0.0 || db < 0.0) {
                    const glm::dvec4 intersection = ca + (cb - ca) * (da / (da - db));

                    if (da < 0.0)
                        ca = intersection;
                    else
                        cb = intersection;
                }
            }

            if (ca.w <= 0.0 || cb.w <= 0.0)
                return;

            auto screen = [&](const glm::dvec4 &p) {
                const glm::dvec3 ndc = glm::dvec3(p) / p.w;

                return ImVec2(float(viewportMin.x + (ndc.x * 0.5 + 0.5) * width), float(viewportMin.y + (0.5 - ndc.y * 0.5) * height));
            };

            drawList->AddLine(screen(ca), screen(cb), IM_COL32(0, 204, 255, 255), 2.0f);
        };

        auto circle = [&](glm::dvec3 center, glm::dvec3 u, glm::dvec3 v) {
            constexpr int segments = 64;
            constexpr double tau = 6.283185307179586;

            for (int i = 0; i < segments; ++i) {
                const double a = tau * i / segments;
                const double b = tau * (i + 1) / segments;

                line(center + u * std::cos(a) + v * std::sin(a), center + u * std::cos(b) + v * std::sin(b));
            }
        };

        const auto &c = world.geometry;

        if (c.shape == Shape::Box) {
            std::array<glm::dvec3, 8> corners{};

            for (int i = 0; i < 8; ++i) {
                corners[i] = glm::dvec3(i & 1 ? 0.5 : -0.5, i & 2 ? 0.5 : -0.5, i & 4 ? 0.5 : -0.5) * glm::dvec3(c.size);
            }

            for (int i = 0; i < 8; ++i) {
                for (int bit : {1, 2, 4}) {
                    if (!(i & bit))
                        line(corners[i], corners[i | bit]);
                }
            }
        } else if (c.shape == Shape::Sphere) {
            const double r = c.radius;

            circle({0, 0, 0}, {r, 0, 0}, {0, r, 0});
            circle({0, 0, 0}, {r, 0, 0}, {0, 0, r});
            circle({0, 0, 0}, {0, r, 0}, {0, 0, r});
        } else {
            const double r = c.radius;
            const double h = c.height * 0.5;

            circle({0, -h, 0}, {r, 0, 0}, {0, 0, r});
            circle({0, h, 0}, {r, 0, 0}, {0, 0, r});

            line({r, -h, 0}, {r, h, 0});
            line({-r, -h, 0}, {-r, h, 0});
            line({0, -h, r}, {0, h, r});
            line({0, -h, -r}, {0, h, -r});
        }

        // handles
        const std::vector<Handle> handles = GetHandles(world);
        constexpr float handleSize = 12.0f;
        constexpr float half = handleSize * 0.5f;

        for (size_t i = 0; i < handles.size(); ++i) {
            glm::vec2 screen{0.0f, 0.0f};

            if (!ToScreen(matrix, handles[i].localPosition, viewportMin, viewportMax, screen))
                continue;

            const bool unusableAxis = AxisFacing(world, handles[i], viewDirection) > unusable;

            const ImU32 fill = handles[i].type == HandleType::Translate ? IM_COL32(0, 204, 255, 255) : static_cast<int>(i) == hovered ? IM_COL32(77, 255, 128, 255) : IM_COL32(255, 255, 255, 255);

            const ImU32 drawColor = unusableAxis ? IM_COL32(255, 255, 255, 60) : fill;

            drawList->AddRectFilled({screen.x - half - 1.0f, screen.y - half - 1.0f}, {screen.x + half + 1.0f, screen.y + half + 1.0f}, IM_COL32(26, 26, 26, unusableAxis ? 20 : 255));
            drawList->AddRectFilled({screen.x - half, screen.y - half}, {screen.x + half, screen.y + half}, drawColor);
        }

        drawList->PopClipRect();
    }

} // namespace Editor::ColliderGizmo
