#include "VertexArray.h"

#include <glm/glm.hpp>

VertexArray::VertexArray() {
    glGenVertexArrays(1, &m_ID);
}

VertexArray::~VertexArray() {
    glDeleteVertexArrays(1, &m_ID);
}

void VertexArray::AddBuffer(const VertexBuffer &vb, const VertexBufferLayout &layout) {
    Bind();
    vb.Bind();
    const auto &elements = layout.GetElements();
    unsigned int offset = 0;

    for (unsigned int i = 0; i < elements.size(); i++) {
        const auto &element = elements[i];
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, element.count, element.type, element.normalized,
                              layout.GetStride(), reinterpret_cast<const void *>(offset));
        offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
    }
}

void VertexArray::AddInstancedBuffer(const VertexBuffer &vb, unsigned int attributeStartLoc) const {
    Bind();
    vb.Bind();
    std::size_t vec4size = sizeof(glm::vec4);
    std::size_t stride = sizeof(glm::mat4);

    for (unsigned int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(attributeStartLoc + i);
        glVertexAttribPointer(attributeStartLoc + i, 4,
                              GL_FLOAT,
                              GL_FALSE,
                              stride,
                              (const void *) (i * vec4size));
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
