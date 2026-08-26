#include "MeshFactory.h"

#include "MeshUtils.h"
#include "Core/Constants.h"
#include <cmath>

#include "IO/Loaders/AssetLoader.h"

namespace Rendering::MeshFactory {
    MeshData CreateQuad() {
        MeshData data;

        data.vertices.push_back({.Position = {-0.5f, -0.5f, 0.0f}, .UV = {0.0f, 0.0f}});
        data.vertices.push_back({.Position = {0.5f, -0.5f, 0.0f}, .UV = {1.0f, 0.0f}});
        data.vertices.push_back({.Position = {0.5f, 0.5f, 0.0f}, .UV = {1.0f, 1.0f}});
        data.vertices.push_back({.Position = {-0.5f, 0.5f, 0.0f}, .UV = {0.0f, 1.0f}});
        data.indices.push_back(0);
        data.indices.push_back(1);
        data.indices.push_back(2);
        data.indices.push_back(2);
        data.indices.push_back(3);
        data.indices.push_back(0);

        return data;
    }

    MeshData CreatePointTopHex(const float size) {
        MeshData data;

        data.vertices.push_back({.Position = {0.0f, 0.0f, 0.0f}, .UV = {0.5f, 0.5f}});

        for (int i = 0; i < 6; i++) {
            const float angle = (i * 60.0f - 90.0f) * Core::PI / 180.0f;

            float x = std::cos(angle) * size;
            float y = std::sin(angle) * size;

            const glm::vec2 uv = glm::vec2(x, y) * 0.5f + 0.5f;

            data.vertices.push_back({.Position = {x, y, 0.0f}, .UV = uv});
        }

        for (uint32_t i = 1; i <= 6; i++) {
            uint32_t next = i % 6 + 1;

            data.indices.push_back(0);
            data.indices.push_back(i);
            data.indices.push_back(next);
        }

        return data;
    }

    MeshData CreateEquiTriangle(const float height = 0.5) {
        MeshData data;

        const float halfBase = height / std::sqrt(3.0f);
        const float centroidOffset = height / 3.0f; // centroid is h/3 above base

        data.vertices.push_back({.Position = {-halfBase, -centroidOffset, 0.0f}, .UV = {0.0f, 0.0f}});

        data.vertices.push_back({.Position = {halfBase, -centroidOffset, 0.0f}, .UV = {1.0f, 0.0f}});

        data.vertices.push_back({.Position = {0.0f, height - centroidOffset, 0.0f}, .UV = {0.5f, 1.0f}});
        data.indices.push_back(0);
        data.indices.push_back(1);
        data.indices.push_back(2);

        return data;
    }


    MeshData CreateEllipse(float radX, float radY, const unsigned int segments) {
        MeshData data;

        if (radX == 0) {
            radX = 1;
        }
        if (radY == 0) {
            radY = 1;
        }

        data.vertices.push_back({.Position = {0.0f, 0.0f, 0.0f}, .UV = {0.5f, 0.5f}});

        for (unsigned int i = 0; i < segments; i++) {
            const float theta = 2.0f * Core::PI * i / segments;

            float x = radX * cosf(theta);
            float y = radY * sinf(theta);

            data.vertices.push_back({.Position = {x, y, 0.0f}, .UV = {x / (2.0f * radX) + 0.5f, y / (2.0f * radY) + 0.5f}});
        }

        for (unsigned int i = 0; i < segments; i++) {
            uint32_t curr = i + 1;
            uint32_t next = (i + 1) % segments + 1;

            data.indices.push_back(0);
            data.indices.push_back(curr);
            data.indices.push_back(next);
        }

        return data;
    }

    MeshData CreateRegularPolygon(const unsigned int sides, const float radius) {
        MeshData data;

        if (sides < 3)
            return data;

        data.vertices.push_back({.Position = {0.0f, 0.0f, 0.0f}, .UV = {0.5f, 0.5f}});

        for (unsigned int i = 0; i < sides; i++) {
            const float theta = 2.0f * Core::PI * i / sides;

            float x = radius * cosf(theta);
            float y = radius * sinf(theta);

            data.vertices.push_back({.Position = {x, y, 0.0f}, .UV = {x / (2.0f * radius) + 0.5f, y / (2.0f * radius) + 0.5f}});
        }

        for (unsigned int i = 0; i < sides; i++) {
            uint32_t curr = i + 1;
            uint32_t next = (i + 1) % sides + 1;

            data.indices.push_back(0);
            data.indices.push_back(curr);
            data.indices.push_back(next);
        }

        return data;
    }

    MeshData CreateRing(const float innerRadius, const float outerRadius, const unsigned int segments) {
        MeshData data;

        for (unsigned int i = 0; i < segments; i++) {
            const float theta = 2.0f * Core::PI * i / segments;

            const float c = cosf(theta);
            const float s = sinf(theta);

            data.vertices.push_back({.Position = {c * innerRadius, s * innerRadius, 0.0f}, .UV = {0.0f, 0.0f}});

            data.vertices.push_back({.Position = {c * outerRadius, s * outerRadius, 0.0f}, .UV = {1.0f, 1.0f}});
        }

        for (unsigned int i = 0; i < segments; i++) {
            const uint32_t next = (i + 1) % segments;

            uint32_t i0 = i * 2;
            uint32_t i1 = i * 2 + 1;
            uint32_t i2 = next * 2;
            uint32_t i3 = next * 2 + 1;

            data.indices.insert(data.indices.end(), {i0, i1, i3, i0, i3, i2});
        }

        return data;
    }

    MeshData CreateSector(const float radius, const float startAngle, const float endAngle, const unsigned int segments) {
        MeshData data;

        data.vertices.push_back({.Position = {0.0f, 0.0f, 0.0f}, .UV = {0.5f, 0.5f}});

        for (unsigned int i = 0; i <= segments; i++) {
            const float t = static_cast<float>(i) / segments;
            const float theta = startAngle + (endAngle - startAngle) * t;

            float x = radius * cosf(theta);
            float y = radius * sinf(theta);

            data.vertices.push_back({.Position = {x, y, 0.0f}, .UV = {x / (2.0f * radius) + 0.5f, y / (2.0f * radius) + 0.5f}});
        }

        for (unsigned int i = 1; i <= segments; i++) {
            data.indices.push_back(0);
            data.indices.push_back(i);
            data.indices.push_back(i + 1);
        }

        return data;
    }

    MeshData CreateDiamond(const float width, const float height) {
        MeshData data;

        float hw = width * 0.5f;
        float hh = height * 0.5f;

        data.vertices = {{.Position = {0.0f, hh, 0.0f}, .UV = {0.5f, 1.0f}},
                         {.Position = {hw, 0.0f, 0.0f}, .UV = {1.0f, 0.5f}},
                         {.Position = {0.0f, -hh, 0.0f}, .UV = {0.5f, 0.0f}},
                         {.Position = {-hw, 0.0f, 0.0f}, .UV = {0.0f, 0.5f}}};

        data.indices = {0, 1, 2, 2, 3, 0};

        return data;
    }

    MeshData CreateCustomMesh2D(const std::vector<glm::vec2> &points) {
        MeshData data;
        if (points.size() < 3) {
            return data;
        }
        glm::vec2 min = points[0];
        glm::vec2 max = points[0];

        for (const auto &point : points) {
            min = glm::min(min, point);
            max = glm::max(max, point);
        }
        data.vertices.reserve(points.size());
        for (const auto &point : points) {
            data.vertices.push_back({.Position = glm::vec3(point, 0.0f), .UV = MeshUtils::GenerateUV(point, min, max)});
        }
        data.indices = MeshUtils::Triangulate(points);

        return data;
    }

    void AppendMesh(MeshData &dst, const MeshData &src) {
        const auto vertexOffset = static_cast<uint32_t>(dst.vertices.size());
        dst.vertices.insert(dst.vertices.end(), src.vertices.begin(), src.vertices.end());
        for (const uint32_t index : src.indices) {
            dst.indices.push_back(index + vertexOffset);
        }
    }

    void RegisterAllMeshFactories() {
        IO::AssetLoader::RegisterMeshFactory("Quad", []() -> std::shared_ptr<Mesh> { return std::make_shared<Mesh>(CreateQuad()); });

        IO::AssetLoader::RegisterMeshFactory("PointTopHex", []() -> std::shared_ptr<Mesh> { return std::make_shared<Mesh>(CreatePointTopHex(0.5f)); });

        IO::AssetLoader::RegisterMeshFactory("ETriang", []() -> std::shared_ptr<Mesh> { return std::make_shared<Mesh>(CreateEquiTriangle(0.5f)); });

        IO::AssetLoader::RegisterMeshFactory("Ellipse", []() -> std::shared_ptr<Mesh> { return std::make_shared<Mesh>(CreateEllipse(0.5f, 0.5f, 50)); });

        IO::AssetLoader::RegisterMeshFactory("Circle", []() -> std::shared_ptr<Mesh> { return std::make_shared<Mesh>(CreateEllipse(0.5f, 0.5f, 50)); });

        IO::AssetLoader::RegisterMeshFactory("Pentagon", []() -> std::shared_ptr<Mesh> { return std::make_shared<Mesh>(CreateRegularPolygon(5, 0.5f)); });

        IO::AssetLoader::RegisterMeshFactory("Hexagon", []() -> std::shared_ptr<Mesh> { return std::make_shared<Mesh>(CreateRegularPolygon(6, 0.5f)); });

        IO::AssetLoader::RegisterMeshFactory("Octagon", []() -> std::shared_ptr<Mesh> { return std::make_shared<Mesh>(CreateRegularPolygon(8, 0.5f)); });

        IO::AssetLoader::RegisterMeshFactory("Ring", []() -> std::shared_ptr<Mesh> { return std::make_shared<Mesh>(CreateRing(0.3f, 0.5f, 50)); });

        IO::AssetLoader::RegisterMeshFactory("Sector", []() -> std::shared_ptr<Mesh> { return std::make_shared<Mesh>(CreateSector(0.5f, 0.0f, Core::PI * 0.5f, 32)); });

        IO::AssetLoader::RegisterMeshFactory("Diamond", []() -> std::shared_ptr<Mesh> { return std::make_shared<Mesh>(CreateDiamond(1.0f, 1.0f)); });
    }
} // namespace Rendering::MeshFactory
