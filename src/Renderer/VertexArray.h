#pragma once

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

    void AddBuffer(const VertexBuffer &vb, const VertexBufferLayout &layout) const;

    void AddInstancedBuffer(const VertexBuffer &vb, unsigned int attributeStartLoc) const;

    void SetIndexBuffer(const IndexBuffer &ibo) const;

    void Bind() const;

    static void Unbind();

    [[nodiscard]] GLuint GetID() const { return m_ID; }

private:
    GLuint m_ID;
};


