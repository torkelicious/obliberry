

#ifndef OBLIBERRY_VERTEXARRAY_H
#define OBLIBERRY_VERTEXARRAY_H
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "glad/glad.h"


class VertexArray {
public:
    // disable copying
    VertexArray(const VertexArray &) = delete;

    VertexArray &operator=(const VertexArray &) = delete;

    // allow moving
    VertexArray(VertexArray &&) = default;

    VertexArray &operator=(VertexArray &&) = default;

    VertexArray();

    ~VertexArray();

    void AddBuffer(const VertexBuffer &vb, const VertexBufferLayout &layout);

    void AddInstancedBuffer(const VertexBuffer &vb, unsigned int attributeStartLoc) const;

    void SetIndexBuffer(const IndexBuffer &ibo) const;

    void Bind() const;

    void Unbind() const;

    [[nodiscard]] GLuint GetID() const { return m_ID; }

private:
    GLuint m_ID;
};


#endif //OBLIBERRY_VERTEXARRAY_H
