#pragma once

#include "glad/glad.h"


namespace Rendering {
    class IndexBuffer {
    public:
        // disable copying
        IndexBuffer(const IndexBuffer &) = delete;

        IndexBuffer &operator=(const IndexBuffer &) = delete;

        // allow moving
        IndexBuffer(IndexBuffer &&other) noexcept : m_ID(other.m_ID), m_Count(other.m_Count) {
            other.m_ID = 0;
            other.m_Count = 0;
        }

        IndexBuffer &operator=(IndexBuffer &&other) noexcept {
            if (this != &other) {
                if (m_ID != 0)
                    glDeleteBuffers(1, &m_ID);
                m_ID = other.m_ID;
                m_Count = other.m_Count;
                other.m_ID = 0;
                other.m_Count = 0;
            }
            return *this;
        }

        IndexBuffer() {}

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
} // namespace Rendering
