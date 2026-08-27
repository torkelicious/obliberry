#pragma once

#include <algorithm>
#include <vector>
#include <cmath>
#include <ranges>
#include <numeric>
#include <glm/glm.hpp>

namespace Rendering::MeshUtils {
    //
    // I failed trigonometry so this may be a lil fucked
    //
    constexpr float EPSILON = 0.00001f;

    // the signed area of a 2D polygon
    [[nodiscard]] inline float SignedArea(const std::vector<glm::vec2> &points) {
        float area = 0.0f;
        for (size_t i = 0; i < points.size(); ++i) {
            const auto &a = points[i];
            const auto &b = points[(i + 1) % points.size()];
            area += a.x * b.y - b.x * a.y;
        }
        return area * 0.5f;
    }

    // the 2D cross product for orientation/convexity checks
    [[nodiscard]] inline float Orientation(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c) { return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x); }

    // UV coordinate for a point within a bounding box
    [[nodiscard]] inline glm::vec2 GenerateUV(const glm::vec2 &point, const glm::vec2 &min, const glm::vec2 &max) {
        const glm::vec2 size = max - min;
        const float u = (size.x != 0.0f) ? (point.x - min.x) / size.x : 0.0f;
        const float v = (size.y != 0.0f) ? (point.y - min.y) / size.y : 0.0f;
        return {u, v};
    }

    [[nodiscard]] inline bool PointInTriangle(const glm::vec2 &p, const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c) {
        const float c1 = Orientation(a, b, p);
        const float c2 = Orientation(b, c, p);
        const float c3 = Orientation(c, a, p);
        return c1 >= -EPSILON && c2 >= -EPSILON && c3 >= -EPSILON;
    }

    // checks if a vertex forms an "ear" in the polygon.
    // an 'ear' is a triangle made from three consecutive vertices of a polygon that lies completely inside the polygon and contains no other vertices

    [[nodiscard]] inline bool IsEar(const size_t index, const std::vector<uint32_t> &polygon_indices, const std::vector<glm::vec2> &points) {
        const size_t count = polygon_indices.size();
        if (count < 3)
            return false;

        const uint32_t prev_idx = polygon_indices[(index + count - 1) % count];
        const uint32_t curr_idx = polygon_indices[index];
        const uint32_t next_idx = polygon_indices[(index + 1) % count];

        const glm::vec2 &a = points[prev_idx];
        const glm::vec2 &b = points[curr_idx];
        const glm::vec2 &c = points[next_idx];

        // check for convexity
        if (Orientation(a, b, c) <= EPSILON) {
            return false;
        }

        // check if any other polygon vertex lies inside this potential triangle
        for (size_t i = 0; i < count; ++i) {
            const uint32_t test_point_idx = polygon_indices[i];
            if (test_point_idx == prev_idx || test_point_idx == curr_idx || test_point_idx == next_idx) {
                continue;
            }

            if (PointInTriangle(points[test_point_idx], a, b, c)) {
                return false;
            }
        }

        return true;
    }

    // triangulates a simple polygon using ear clipping algorithm
    [[nodiscard]] inline std::vector<uint32_t> Triangulate(std::vector<glm::vec2> points) {
        std::vector<uint32_t> indices;
        const size_t num_points = points.size();

        if (num_points < 3) {
            return indices;
        }

        // track the original index of each point through the reversal
        std::vector<uint32_t> original_index(num_points);
        std::iota(original_index.begin(), original_index.end(), 0);

        // counter clockwise winding order
        if (SignedArea(points) < 0.0f) {
            std::ranges::reverse(points);
            std::ranges::reverse(original_index);
        }

        std::vector<uint32_t> polygon_indices(num_points);
        std::iota(polygon_indices.begin(), polygon_indices.end(), 0);
        int current_vertex_index = 0;
        int iterations = 0; // no infinite loops!!!!
        const int max_iterations = num_points * num_points;

        while (polygon_indices.size() > 3 && iterations < max_iterations) {
            iterations++;
            const size_t current_polygon_size = polygon_indices.size();
            current_vertex_index %= current_polygon_size;

            if (IsEar(current_vertex_index, polygon_indices, points)) {
                const uint32_t prev_idx = polygon_indices[(current_vertex_index + current_polygon_size - 1) % current_polygon_size];
                const uint32_t curr_idx = polygon_indices[current_vertex_index];
                const uint32_t next_idx = polygon_indices[(current_vertex_index + 1) % current_polygon_size];

                indices.push_back(original_index[prev_idx]);
                indices.push_back(original_index[curr_idx]);
                indices.push_back(original_index[next_idx]);
                polygon_indices.erase(polygon_indices.begin() + current_vertex_index);
            } else {
                current_vertex_index++;
            }
        }

        if (polygon_indices.size() == 3) {
            indices.push_back(original_index[polygon_indices[0]]);
            indices.push_back(original_index[polygon_indices[1]]);
            indices.push_back(original_index[polygon_indices[2]]);
        } else if (iterations >= max_iterations) {
            // failed (such as self-intersecting polygon etc)
            indices.clear();
        }

        return indices;
    }

    inline void NormalizeMesh(MeshData &mesh, const float targetSize = 1.0f) {
        if (mesh.vertices.empty()) {
            return;
        }

        glm::vec3 min = mesh.vertices[0].Position;
        glm::vec3 max = mesh.vertices[0].Position;

        for (const auto &vertex : mesh.vertices) {
            min = glm::min(min, vertex.Position);
            max = glm::max(max, vertex.Position);
        }

        const glm::vec3 center = (min + max) * 0.5f;
        const glm::vec3 size = max - min;
        const float largestDimension = std::max({size.x, size.y, size.z});

        if (largestDimension <= 0.0f) {
            return;
        }

        const float scale = targetSize / largestDimension;
        for (auto &vertex : mesh.vertices) {
            vertex.Position = (vertex.Position - center) * scale;
        }
    }

} // namespace Rendering::MeshUtils
