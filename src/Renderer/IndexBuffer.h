#pragma once

#include "glad/glad.h"


class IndexBuffer {
public:
    // disable copying
    IndexBuffer(const IndexBuffer &) = delete;

    IndexBuffer &operator=(const IndexBuffer &) = delete;

    // allow moving
    IndexBuffer(IndexBuffer &&) = default;

    IndexBuffer &operator=(IndexBuffer &&) = default;

    IndexBuffer() : m_ID(0), m_Count(0) {
    }

    ~IndexBuffer();

    void Init(const unsigned int *data, unsigned int count);

    void Bind() const;

    static void Unbind();

    void SetData(const unsigned int *data, unsigned int count);

    [[nodiscard]] unsigned int GetCount() const { return m_Count; }

private:
    GLuint m_ID = 0;
    unsigned int m_Count = 0;
};
