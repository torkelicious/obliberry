#include "IndexBuffer.h"

namespace Rendering {
    IndexBuffer::~IndexBuffer() {
        if (m_ID != 0) {
            glDeleteBuffers(1, &m_ID);
            m_ID = 0;
        }
    }

    void IndexBuffer::Init(const unsigned int *data, const unsigned int count) {
        m_Count = count;
        glGenBuffers(1, &m_ID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(GLuint), data, GL_STATIC_DRAW);
    }

    void IndexBuffer::Bind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
    }

    void IndexBuffer::Unbind() {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void IndexBuffer::SetData(const unsigned int *data, const unsigned int count) {
        m_Count = count;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(GLuint), data, GL_DYNAMIC_DRAW);
    }
} // namespace Rendering
