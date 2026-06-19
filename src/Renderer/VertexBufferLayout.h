#ifndef OBLIBERRY_VERTEXBUFFERLAYOUT_H
#define OBLIBERRY_VERTEXBUFFERLAYOUT_H

#include <vector>
#include "glad/glad.h"

struct VertexBufferElement {
    unsigned int type;
    unsigned int count;
    unsigned int normalized;

    static unsigned int GetSizeOfType(const unsigned int type) {
        switch (type) {
            case GL_FLOAT:
                return 4;
            case GL_UNSIGNED_INT:
                return 4;
            case GL_UNSIGNED_BYTE:
                return 1;
            default: return 0;
        }
    }
};

class VertexBufferLayout {
private:
    std::vector<VertexBufferElement> m_Elements;
    unsigned int m_Stride;

public:
    VertexBufferLayout() : m_Stride(0) {
    }

    void Push(const unsigned int type, const unsigned int count) {
        m_Elements.push_back({type, count, GL_FALSE});
        m_Stride += count * VertexBufferElement::GetSizeOfType(type);
    }

    [[nodiscard]] const std::vector<VertexBufferElement> &GetElements() const { return m_Elements; }
    [[nodiscard]] unsigned int GetStride() const { return m_Stride; }
};

#endif //OBLIBERRY_VERTEXBUFFERLAYOUT_H
