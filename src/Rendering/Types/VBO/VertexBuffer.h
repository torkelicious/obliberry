#pragma once

#include "Rendering/GLDelete.h"
#include "glad/glad.h"
#include <utility>


namespace Rendering {
    class VertexBuffer {
    public:
        // disable copying
        VertexBuffer(const VertexBuffer &) = delete;

        VertexBuffer &operator=(const VertexBuffer &) = delete;

        // allow moving
        VertexBuffer(VertexBuffer &&other) noexcept : m_ID(other.m_ID) { other.m_ID = 0; }

        VertexBuffer &operator=(VertexBuffer &&other) noexcept {
            if (this != &other) {
                QDeleteBuffer(std::exchange(m_ID, 0));

                m_ID = other.m_ID;
                other.m_ID = 0;
            }
            return *this;
        }

        VertexBuffer() {}

        void Init(const void *data, unsigned int size, GLenum usage = GL_STATIC_DRAW);

        ~VertexBuffer();

        void Bind() const;

        static void Unbind();

        void SetData(const void *data, unsigned int size) const;

        void SetSubData(const void *data, unsigned int size, unsigned int offset = 0) const;

        void SetDataOrphaned(const void *data, unsigned int size) const;

    private:
        GLuint m_ID = 0;
    };
} // namespace Rendering
