

#ifndef OBLIBERRY_VERTEXBUFFER_H
#define OBLIBERRY_VERTEXBUFFER_H
#include "glad/glad.h"


class VertexBuffer {
public:
    // disable copying
    VertexBuffer(const VertexBuffer &) = delete;

    VertexBuffer &operator=(const VertexBuffer &) = delete;

    // allow moving
    VertexBuffer(VertexBuffer &&) = default;

    VertexBuffer &operator=(VertexBuffer &&) = default;


    VertexBuffer(const void *data, unsigned int size, GLenum usage = GL_STATIC_DRAW);

    ~VertexBuffer();

    void Bind() const;

    void Unbind() const;

    void SetData(const void *data, unsigned int size);

    void SetSubData(const void *data, unsigned int size, unsigned int offset = 0);

private:
    GLuint m_ID;
};


#endif //OBLIBERRY_VERTEXBUFFER_H
