

#ifndef OBLIBERRY_INDEXBUFFER_H
#define OBLIBERRY_INDEXBUFFER_H
#include "glad/glad.h"


class IndexBuffer {
public:
    // disable copying
    IndexBuffer(const IndexBuffer &) = delete;

    IndexBuffer &operator=(const IndexBuffer &) = delete;

    // allow moving
    IndexBuffer(IndexBuffer &&) = default;

    IndexBuffer &operator=(IndexBuffer &&) = default;

    IndexBuffer();

    IndexBuffer(const unsigned int *data, unsigned int count);

    ~IndexBuffer();

    void Bind() const;

    void Unbind() const;

    void SetData(const unsigned int *data, unsigned int count);

    [[nodiscard]] unsigned int GetCount() const { return m_Count; }

private:
    GLuint m_ID;
    unsigned int m_Count;
};


#endif //OBLIBERRY_INDEXBUFFER_H
