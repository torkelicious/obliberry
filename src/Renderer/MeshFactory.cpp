#include "MeshFactory.h"
#include "Core/Constants.h"
#include <cmath>

#include "IO/AssetLoader.h"

namespace MeshFactory {
    MeshData CreateQuad() {
        MeshData data;

        data.vertices.push_back({{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}});
        data.vertices.push_back({{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}});
        data.vertices.push_back({{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}});
        data.vertices.push_back({{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}});
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

        data.vertices.push_back({{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}});

        for (int i = 0; i < 6; i++) {
            const float angle = (i * 60.0f - 90.0f) * PI / 180.0f;

            float x = std::cos(angle) * size;
            float y = std::sin(angle) * size;

            const glm::vec2 uv = glm::vec2(x, y) * 0.5f + 0.5f;

            data.vertices.push_back({{x, y, 0.0f}, uv});
        }

        for (uint32_t i = 1; i <= 6; i++) {
            uint32_t next = i % 6 + 1;

            data.indices.push_back(0);
            data.indices.push_back(i);
            data.indices.push_back(next);
        }

        return data;
    }

    MeshData CreateEquiTriangle(float height = 0.5) {
        MeshData data;

        data.vertices.push_back({{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}});

        data.vertices.push_back({
            {0.5f, -height, 0.0f},
            {1.0f, 0.0f}
        });

        data.vertices.push_back({
            {0.0f, 0.5f, 0.0f},
            {0.5f, 1.0f}
        });
        data.indices.push_back(0);
        data.indices.push_back(1);
        data.indices.push_back(2);

        return data;
    }


    MeshData CreateEllipse(float radX, float radY, const unsigned int segments) {
        MeshData data;

        if (radX == 0) { radX = 1; }
        if (radY == 0) { radY = 1; }

        data.vertices.push_back({
            {0.0f, 0.0f, 0.0f},
            {0.5f, 0.5f}
        });

        for (unsigned int i = 0; i < segments; i++) {
            float theta = 2.0f * PI * i / segments;

            float x = radX * cosf(theta);
            float y = radY * sinf(theta);

            data.vertices.push_back({
                {x, y, 0.0f},
                {
                    x / (2.0f * radX) + 0.5f,
                    y / (2.0f * radY) + 0.5f
                }
            });
        }

        for (unsigned int i = 0; i < segments; i++) {
            uint32_t curr = i + 1;
            uint32_t next = ((i + 1) % segments) + 1;

            data.indices.push_back(0);
            data.indices.push_back(curr);
            data.indices.push_back(next);
        }

        return data;
    }

    MeshData CreateRegularPolygon(
        unsigned int sides,
        float radius) {
        MeshData data;

        if (sides < 3)
            return data;

        data.vertices.push_back({
            {0.0f, 0.0f, 0.0f},
            {0.5f, 0.5f}
        });

        for (unsigned int i = 0; i < sides; i++) {
            float theta = 2.0f * PI * i / sides;

            float x = radius * cosf(theta);
            float y = radius * sinf(theta);

            data.vertices.push_back({
                {x, y, 0.0f},
                {
                    x / (2.0f * radius) + 0.5f,
                    y / (2.0f * radius) + 0.5f
                }
            });
        }

        for (unsigned int i = 0; i < sides; i++) {
            uint32_t curr = i + 1;
            uint32_t next = ((i + 1) % sides) + 1;

            data.indices.push_back(0);
            data.indices.push_back(curr);
            data.indices.push_back(next);
        }

        return data;
    }

    MeshData CreateRing(
        float innerRadius,
        float outerRadius,
        unsigned int segments) {
        MeshData data;

        for (unsigned int i = 0; i < segments; i++) {
            float theta = 2.0f * PI * i / segments;

            float c = cosf(theta);
            float s = sinf(theta);

            data.vertices.push_back({
                {c * innerRadius, s * innerRadius, 0.0f},
                {0.0f, 0.0f}
            });

            data.vertices.push_back({
                {c * outerRadius, s * outerRadius, 0.0f},
                {1.0f, 1.0f}
            });
        }

        for (unsigned int i = 0; i < segments; i++) {
            uint32_t next = (i + 1) % segments;

            uint32_t i0 = i * 2;
            uint32_t i1 = i * 2 + 1;
            uint32_t i2 = next * 2;
            uint32_t i3 = next * 2 + 1;

            data.indices.insert(
                data.indices.end(),
                {
                    i0, i1, i3,
                    i0, i3, i2
                });
        }

        return data;
    }

    MeshData CreateSector(
        float radius,
        float startAngle,
        float endAngle,
        unsigned int segments) {
        MeshData data;

        data.vertices.push_back({
            {0.0f, 0.0f, 0.0f},
            {0.5f, 0.5f}
        });

        for (unsigned int i = 0; i <= segments; i++) {
            const float t = static_cast<float>(i) / segments;
            const float theta = startAngle + (endAngle - startAngle) * t;

            float x = radius * cosf(theta);
            float y = radius * sinf(theta);

            data.vertices.push_back({
                {x, y, 0.0f},
                {
                    x / (2.0f * radius) + 0.5f,
                    y / (2.0f * radius) + 0.5f
                }
            });
        }

        for (unsigned int i = 1; i <= segments; i++) {
            data.indices.push_back(0);
            data.indices.push_back(i);
            data.indices.push_back(i + 1);
        }

        return data;
    }

    MeshData CreateDiamond(
        const float width,
        const float height) {
        MeshData data;

        float hw = width * 0.5f;
        float hh = height * 0.5f;

        data.vertices = {
            {{0.0f, hh, 0.0f}, {0.5f, 1.0f}},
            {{hw, 0.0f, 0.0f}, {1.0f, 0.5f}},
            {{0.0f, -hh, 0.0f}, {0.5f, 0.0f}},
            {{-hw, 0.0f, 0.0f}, {0.0f, 0.5f}}
        };

        data.indices = {
            0, 1, 2,
            2, 3, 0
        };

        return data;
    }

    void AppendMesh(MeshData &dst, const MeshData &src) {
        const auto vertexOffset = static_cast<uint32_t>(dst.vertices.size());
        dst.vertices.insert(
            dst.vertices.end(),
            src.vertices.begin(),
            src.vertices.end()
        );
        for (uint32_t index: src.indices) {
            dst.indices.push_back(index + vertexOffset);
        }
    }

    void RegisterAllMeshFactories() {
        AssetLoader::RegisterMeshFactory("Quad", []() -> std::shared_ptr<Mesh> {
            auto [vertices, indices] = CreateQuad();
            return std::make_shared<Mesh>(vertices, indices);
        });

        AssetLoader::RegisterMeshFactory("PointTopHex", []() -> std::shared_ptr<Mesh> {
            auto [vertices, indices] = CreatePointTopHex(0.5f);
            return std::make_shared<Mesh>(vertices, indices);
        });

        AssetLoader::RegisterMeshFactory("ETriang", []() -> std::shared_ptr<Mesh> {
            auto [vertices, indices] = CreateEquiTriangle(0.5f);
            return std::make_shared<Mesh>(vertices, indices);
        });

        AssetLoader::RegisterMeshFactory("Ellipse", []() -> std::shared_ptr<Mesh> {
            auto [vertices, indices] = CreateEllipse(0.5f, 0.5f, 50);
            return std::make_shared<Mesh>(vertices, indices);
        });

        AssetLoader::RegisterMeshFactory("Circle", []() -> std::shared_ptr<Mesh> {
            auto [vertices, indices] = CreateEllipse(0.5f, 0.5f, 50);
            return std::make_shared<Mesh>(vertices, indices);
        });

        AssetLoader::RegisterMeshFactory("Pentagon", []() -> std::shared_ptr<Mesh> {
            auto [vertices, indices] = CreateRegularPolygon(5, 0.5f);
            return std::make_shared<Mesh>(vertices, indices);
        });

        AssetLoader::RegisterMeshFactory("Hexagon", []() -> std::shared_ptr<Mesh> {
            auto [vertices, indices] = CreateRegularPolygon(6, 0.5f);
            return std::make_shared<Mesh>(vertices, indices);
        });

        AssetLoader::RegisterMeshFactory("Octagon", []() -> std::shared_ptr<Mesh> {
            auto [vertices, indices] = CreateRegularPolygon(8, 0.5f);
            return std::make_shared<Mesh>(vertices, indices);
        });

        AssetLoader::RegisterMeshFactory("Ring", []() -> std::shared_ptr<Mesh> {
            auto [vertices, indices] = CreateRing(0.3f, 0.5f, 50);
            return std::make_shared<Mesh>(vertices, indices);
        });

        AssetLoader::RegisterMeshFactory("Sector", []() -> std::shared_ptr<Mesh> {
            auto [vertices, indices] =
                    CreateSector(0.5f, 0.0f, PI * 0.5f, 32);
            return std::make_shared<Mesh>(vertices, indices);
        });

        AssetLoader::RegisterMeshFactory("Diamond", []() -> std::shared_ptr<Mesh> {
            auto [vertices, indices] =
                    CreateDiamond(1.0f, 1.0f);
            return std::make_shared<Mesh>(vertices, indices);
        });
    }
}
