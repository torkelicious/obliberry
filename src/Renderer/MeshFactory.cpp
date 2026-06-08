#include "MeshFactory.h"

MeshData MeshFactory::CreatePointTopHex(float size) {
    MeshData data;

    data.vertices.push_back({{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}});

    for (int i = 0; i < 6; i++) {
        constexpr float PI = 3.14159265359f;
        float angle = (i * 60.0f - 90.0f) * PI / 180.0f;

        float x = cos(angle) * size;
        float y = sin(angle) * size;

        glm::vec2 uv = glm::vec2(x, y) * 0.5f + 0.5f;

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

// billlboard for kinda player sprites and stuff
// things may break if you try to rotate this via transform :)
MeshData MeshFactory::CreateStandingQuad(float width, float height) {
    MeshData data;

    // the inverse of the cameras comp rotation (X=55, Z=45).
    glm::vec3 right_vec(0.7071068f, -0.7071068f, 0.0f);
    glm::vec3 up_vec(0.4055798f, 0.4055798f, -0.8191520f);

    float halfWidth = width / 2.0f;

    glm::vec3 bl = -halfWidth * right_vec;
    glm::vec3 br = halfWidth * right_vec;
    glm::vec3 tr = halfWidth * right_vec + height * up_vec;
    glm::vec3 tl = -halfWidth * right_vec + height * up_vec;

    data.vertices.push_back({bl, {0.0f, 0.0f}});
    data.vertices.push_back({br, {1.0f, 0.0f}});
    data.vertices.push_back({tr, {1.0f, 1.0f}});
    data.vertices.push_back({tl, {0.0f, 1.0f}});

    data.indices.push_back(0);
    data.indices.push_back(1);
    data.indices.push_back(2);
    data.indices.push_back(2);
    data.indices.push_back(3);
    data.indices.push_back(0);

    return data;
}

