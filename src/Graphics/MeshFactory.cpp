#include "MeshFactory.h"

#include <cmath>

Mesh MeshFactory::CreateQuad() {
    MeshData data;
    data.vertices =
    {
        {0, 0, 0, 0},
        {1, 0, 1, 0},
        {1, 1, 1, 1},
        {0, 1, 0, 1}
    };

    data.indices = {0, 1, 2, 2, 3, 0};
    Mesh mesh;
    mesh.Upload(data);
    return mesh;
}

Mesh MeshFactory::CreateTriangle() {
    MeshData data;

    data.vertices =
    {
        {0.0f, 0.0f, 0.0f, 0.0f}, // bottom-left
        {1.0f, 0.0f, 1.0f, 0.0f}, // bottom-right
        {0.5f, 1.0f, 0.5f, 1.0f} // top
    };

    data.indices = {0, 1, 2};

    Mesh mesh;
    mesh.Upload(data);
    return mesh;
}

Mesh MeshFactory::CreatePointTopHex(float rad) {
    MeshData data;
    data.vertices.push_back({0.0f, 0.0f, 0.5f, 0.5f}); // center

    for (int i = 0; i < 6; i++) {
        float angle = glm::radians(60.0f * static_cast<float>(i) + 30.0f);

        float c = std::cos(angle);
        float s = std::sin(angle);

        data.vertices.push_back({
            rad * c,
            rad * s,
            0.5f + 0.5f * c,
            0.5f + 0.5f * s
        });
    }

    for (int i = 1; i <= 6; i++) {
        data.indices.push_back(0);
        data.indices.push_back(i);
        data.indices.push_back((i % 6) + 1);
    }

    Mesh mesh;
    mesh.Upload(data);
    return mesh;
}


// stop this code duplicaiton shit later but 4 now this is fine
