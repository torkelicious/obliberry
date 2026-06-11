#include "VertexBuffer.h"


VertexBuffer::VertexBuffer(const void *data, unsigned int size, GLenum usage) {
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

void VertexBuffer::Unbind() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::SetData(const void *data, unsigned int size) {
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
}

void VertexBuffer::SetSubData(const void *data, unsigned int size, unsigned int offset) {
    glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    // modify existing memory instead of reallocating
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}
