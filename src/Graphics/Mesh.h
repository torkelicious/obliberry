

#ifndef ISOMETRICGAME_MESH_H
#define ISOMETRICGAME_MESH_H
#include "glad/glad.h"
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "ECS/Components.h"


struct Vertex {
    float x, y;
    float u, v;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

inline glm::mat4 TransformToMatrix(const Transform &t) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(t.Position, 0.0f));
    model = glm::rotate(model, t.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(t.Scale, 1.0f));
    return model;
}

class Mesh {
public:
    Mesh() = default;

    ~Mesh();

    void Upload(const MeshData &data);

    void Bind() const;

    int GetIndexCount() const { return m_IndexCount; }

private:
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_IBO = 0;
    int m_IndexCount = 0;
};

#endif //ISOMETRICGAME_MESH_H
