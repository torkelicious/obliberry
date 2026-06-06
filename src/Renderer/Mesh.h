#ifndef OBLIBERRY_MESH_H
#define OBLIBERRY_MESH_H

#include <vector>
#include <cstdint>

#include <glm/glm.hpp>

#include "VertexArray.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "IndexBuffer.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec2 UV;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

template<typename T>
struct VertexTraits;

template<>
struct VertexTraits<Vertex> {
    static const VertexBufferLayout &GetLayout() {
        static VertexBufferLayout layout = [] {
            VertexBufferLayout l;
            l.Push(GL_FLOAT, 3);
            l.Push(GL_FLOAT, 2);
            return l;
        }();

        return layout;
    }
};

class Mesh {
public:
    // disable copying
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // allow moving
    Mesh(Mesh&&) = default;
    Mesh& operator=(Mesh&&) = default;




    Mesh(const MeshData &data)
        : m_VBO(
              data.vertices.data(),
              static_cast<uint32_t>(data.vertices.size() * sizeof(Vertex))
          ),
          m_IBO(
              data.indices.data(),
              static_cast<uint32_t>(data.indices.size())
          ) {
        const auto layout = VertexTraits<Vertex>::GetLayout();
        m_VAO.Bind();
        m_VBO.Bind();
        m_VAO.AddBuffer(m_VBO, layout);
        m_IBO.Bind();
    }

    template<typename TVertex>
    Mesh(
        const std::vector<TVertex> &vertices,
        const std::vector<uint32_t> &indices)
        : m_VBO(
              vertices.data(),
              static_cast<uint32_t>(vertices.size() * sizeof(TVertex))
          ),
          m_IBO(
              indices.data(),
              static_cast<uint32_t>(indices.size())
          ) {
        const auto layout = VertexTraits<TVertex>::GetLayout();
        m_VAO.Bind();
        m_VBO.Bind();
        m_VAO.AddBuffer(m_VBO, layout);
        m_IBO.Bind();
    }

    void Upload(const MeshData &data) {
        m_VAO.Bind();
        m_VBO.Bind();
        m_VBO.SetData(data.vertices.data(), static_cast<uint32_t>(data.vertices.size() * sizeof(Vertex)));
        m_IBO.Bind();
        m_IBO.SetData(data.indices.data(), static_cast<uint32_t>(data.indices.size()));
    }

    void Bind() const {
        m_VAO.Bind();
    }

    uint32_t GetIndexCount() const {
        return m_IBO.GetCount();
    }

private:
    VertexArray m_VAO;
    VertexBuffer m_VBO;
    IndexBuffer m_IBO;
};

#endif
