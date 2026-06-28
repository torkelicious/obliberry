#pragma once

#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "glad/glad.h"

class VertexArray {
public:
    // disable copying
    VertexArray(const VertexArray&) = delete;

    VertexArray& operator=(const VertexArray&) = delete;

    // allow moving
    VertexArray(VertexArray&& other) noexcept : m_ID(other.m_ID) { other.m_ID = 0; }

    VertexArray& operator=(VertexArray&& other) noexcept {
        if (this != &other) {
            if (m_ID != 0)
                glDeleteVertexArrays(1, &m_ID);
            m_ID = other.m_ID;
            other.m_ID = 0;
        }
        return *this;
    }

    VertexArray() : m_ID(0) {}

    ~VertexArray();

    void Init();

    void AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout) const;

    void AddInstancedBuffer(const VertexBuffer& vb, unsigned int attributeStartLoc) const;
    void AddInstancedIntBuffer(const VertexBuffer& vb, unsigned int attributeLoc) const;

    void SetIndexBuffer(const IndexBuffer& ibo) const;

    void Bind() const;

    static void Unbind();

    [[nodiscard]] GLuint GetID() const { return m_ID; }

private:
    GLuint m_ID;
};
