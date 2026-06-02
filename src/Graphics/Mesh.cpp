#include "Mesh.h"

Mesh::~Mesh()
{
    if (m_VAO)
        glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO)
        glDeleteBuffers(1, &m_VBO);
    if (m_IBO)
        glDeleteBuffers(1, &m_IBO);
}

void Mesh::Bind() const {
    glBindVertexArray(m_VAO);
}


void Mesh::Upload(const MeshData &data) {
    m_IndexCount = (int)data.indices.size();

    glGenVertexArrays(1,&m_VAO);
    glGenBuffers(1,&m_VBO);
    glGenBuffers(1,&m_IBO);

    glBindVertexArray(m_VAO);

    // vbo
    glBindBuffer(GL_ARRAY_BUFFER,m_VBO);
    glBufferData(GL_ARRAY_BUFFER,
        data.vertices.size() * sizeof(Vertex),
        data.vertices.data(),
        GL_STATIC_DRAW);

    // ibo
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m_IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        data.indices.size() * sizeof(uint32_t),
        data.indices.data(),
        GL_STATIC_DRAW);

    // positioning (loc 0)
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)0);

    glEnableVertexAttribArray(0);

    // UV (loc 1)
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)(2*sizeof(float))
        );
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}
