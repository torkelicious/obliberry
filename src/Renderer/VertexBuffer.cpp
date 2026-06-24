#include "VertexBuffer.h"


void VertexBuffer::Init(const void *data, const unsigned int size, const GLenum usage) {
    glGenBuffers(1, &m_ID);
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
}

VertexBuffer::~VertexBuffer() {
    glDeleteBuffers(1, &m_ID);
}

void VertexBuffer::Bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
}

void VertexBuffer::Unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::SetData(const void *data, const unsigned int size) const {
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
}

void VertexBuffer::SetSubData(const void *data, const unsigned int size, const unsigned int offset) const {
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    // modify existing memory instead of reallocating
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}

void VertexBuffer::SetDataOrphaned(const void *data, const unsigned int size) const {
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}
