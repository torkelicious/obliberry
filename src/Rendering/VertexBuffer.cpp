#include "VertexBuffer.h"

void Rendering::VertexBuffer::Init(const void *data, const unsigned int size, const GLenum usage) {
    glGenBuffers(1, &m_ID);
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
}

Rendering::VertexBuffer::~VertexBuffer() { glDeleteBuffers(1, &m_ID); }

void Rendering::VertexBuffer::Bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_ID); }

void Rendering::VertexBuffer::Unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

void Rendering::VertexBuffer::SetData(const void *data, const unsigned int size) const {
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
}

void Rendering::VertexBuffer::SetSubData(const void *data, const unsigned int size, const unsigned int offset) const {
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    // modify existing memory instead of reallocating
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}

void Rendering::VertexBuffer::SetDataOrphaned(const void *data, const unsigned int size) const {
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
}
