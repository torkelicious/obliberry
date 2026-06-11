#include "MeshFactory.h"

#include <cmath>

MeshData MeshFactory::CreatePointTopHex(const float size) {
    MeshData data;

    data.vertices.push_back({{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}});

    for (int i = 0; i < 6; i++) {
        constexpr float PI = 3.14159265359f;
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

MeshData MeshFactory::CreateQuad() {
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

MeshData MeshFactory::CreateTriangle() {
    MeshData data;

    data.vertices.push_back({{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}});
    data.vertices.push_back({{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}});
    data.vertices.push_back({{0.0f, 0.5f, 0.0f}, {0.5f, 1.0f}});
    data.indices.push_back(0);
    data.indices.push_back(1);
    data.indices.push_back(2);

    return data;
}