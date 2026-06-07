

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


    VertexBuffer(const void *data, unsigned int size);

    ~VertexBuffer();

    void Bind() const;

    void Unbind() const;

    void SetData(const void *data, unsigned int size);

private:
    GLuint m_ID;
};


#endif //OBLIBERRY_VERTEXBUFFER_H
