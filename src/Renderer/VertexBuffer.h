#pragma once

#include "glad/glad.h"


class VertexBuffer {
public:
    // disable copying
    VertexBuffer(const VertexBuffer &) = delete;

    VertexBuffer &operator=(const VertexBuffer &) = delete;

    // allow moving
    VertexBuffer(VertexBuffer &&) = default;

    VertexBuffer &operator=(VertexBuffer &&) = default;


    VertexBuffer() : m_ID(0) {
    }

    void Init(const void *data, unsigned int size, GLenum usage = GL_STATIC_DRAW);

    ~VertexBuffer();

    void Bind() const;

    static void Unbind();

    void SetData(const void *data, unsigned int size) const;

    void SetSubData(const void *data, unsigned int size, unsigned int offset = 0) const;

private:
    GLuint m_ID = 0;
};
