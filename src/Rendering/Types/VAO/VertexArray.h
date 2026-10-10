#pragma once

#include "Rendering/GLDelete.h"
#include "Rendering/Types/IBO/IndexBuffer.h"
#include "Rendering/Types/VBO/VertexBuffer.h"
#include "Rendering/Types/VBO/VertexBufferLayout.h"
#include "glad/glad.h"
#include <utility>

namespace Rendering {
    class VertexArray {
    public:
        // disable copying
        VertexArray(const VertexArray &) = delete;

        VertexArray &operator=(const VertexArray &) = delete;

        // allow moving
        VertexArray(VertexArray &&other) noexcept : m_ID(other.m_ID) { other.m_ID = 0; }

        VertexArray &operator=(VertexArray &&other) noexcept {
            if (this != &other) {
                QDeleteVertexArray(std::exchange(m_ID, 0));

                m_ID = other.m_ID;
                other.m_ID = 0;
            }
            return *this;
        }

        VertexArray() : m_ID(0) {}

        ~VertexArray();

        void Init();

        void AddBuffer(const VertexBuffer &vb, const VertexBufferLayout &layout) const;

        void AddInstancedBuffer(const VertexBuffer &vb, unsigned int attributeStartLoc) const;

        void AddInstancedIntBuffer(const VertexBuffer &vb, unsigned int attributeLoc) const;

        void SetIndexBuffer(const IndexBuffer &ibo) const;

        void Bind() const;

        static void Unbind();

        [[nodiscard]] GLuint GetID() const { return m_ID; }

    private:
        GLuint m_ID;
    };
} // namespace Rendering
