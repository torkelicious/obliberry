#include "VertexArray.h"

#include <glm/glm.hpp>

VertexArray::VertexArray() {
    glGenVertexArrays(1, &m_ID);
}

VertexArray::~VertexArray() {
    glDeleteVertexArrays(1, &m_ID);
}

void VertexArray::AddBuffer(const VertexBuffer &vb, const VertexBufferLayout &layout) const {
    Bind();
    vb.Bind();
    const auto &elements = layout.GetElements();
    unsigned int offset = 0;

    for (unsigned int i = 0; i < elements.size(); i++) {
        const auto &[type, count, normalized] = elements[i];
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, count, type, normalized,
                              layout.GetStride(), reinterpret_cast<const void *>(offset));
        offset += count * VertexBufferElement::GetSizeOfType(type);
    }
}

void VertexArray::AddInstancedBuffer(const VertexBuffer &vb, const unsigned int attributeStartLoc) const {
    Bind();
    vb.Bind();

    for (unsigned int i = 0; i < 4; i++) {
        constexpr std::size_t stride = sizeof(glm::mat4);
        constexpr std::size_t vec4size = sizeof(glm::vec4);
        glEnableVertexAttribArray(attributeStartLoc + i);
        glVertexAttribPointer(attributeStartLoc + i, 4,
                              GL_FLOAT,
                              GL_FALSE,
                              stride,
                              reinterpret_cast<const void *>(i * vec4size));
        glVertexAttribDivisor(attributeStartLoc + i, 1);
    }
}

void VertexArray::SetIndexBuffer(const IndexBuffer &ibo) const {
    Bind();
    ibo.Bind();
}

void VertexArray::Bind() const {
    glBindVertexArray(m_ID);
}

void VertexArray::Unbind() const {
    glBindVertexArray(0);
}
