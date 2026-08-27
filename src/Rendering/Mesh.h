#pragma once


#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "VertexArray.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "IndexBuffer.h"

namespace Rendering {
    struct Vertex {
        glm::vec3 Position;
        glm::vec2 UV;
    };

    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    template <typename T> struct VertexTraits;

    template <> struct VertexTraits<Vertex> {
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
        Mesh(const Mesh &) = delete;

        Mesh &operator=(const Mesh &) = delete;

        // allow moving
        Mesh(Mesh &&) = default;

        Mesh &operator=(Mesh &&) = default;

        explicit Mesh(MeshData data) : m_TempData(std::move(data)) { ComputeBoundingSphere(); }

        void InitGL() {
            m_VBO.Init(m_TempData.vertices.data(), static_cast<uint32_t>(m_TempData.vertices.size() * sizeof(Vertex)));
            m_IBO.Init(m_TempData.indices.data(), static_cast<uint32_t>(m_TempData.indices.size()));
            // free the RAM copy once uploaded
            m_TempData.vertices.clear();
            m_TempData.indices.clear();
        }

        void ComputeBoundingSphere() noexcept {
            m_BoundingRadius = 0.0f;
            for (const auto &[Position, UV] : m_TempData.vertices) {
                if (const float dist = glm::length(Position); dist > m_BoundingRadius)
                    m_BoundingRadius = dist;
            }
        }

        [[nodiscard]] float GetBoundingRadius() const noexcept { return m_BoundingRadius; }

        void Upload(const MeshData &data) {
            m_VAO.Bind();
            m_VBO.SetData(data.vertices.data(), static_cast<uint32_t>(data.vertices.size() * sizeof(Vertex)));
            m_IBO.SetData(data.indices.data(), static_cast<uint32_t>(data.indices.size()));
            // unbind to avoid polluted state
            glBindVertexArray(0);
        }

        void Bind() const { m_VAO.Bind(); }

        // store data untill save-time
        void CustomDataStore(const MeshData &data) {
            m_CustomDataBin = data;
            isCustom = true;
        }

        [[nodiscard]] MeshData GetCustomData() const { return m_CustomDataBin; }
        void ClearCustomDataStore() {
            m_CustomDataBin = {};
            isCustom = false;
        }

        [[nodiscard]] bool IsCustom() const { return isCustom; }

        [[nodiscard]] const VertexArray &GetVertexArray() const { return m_VAO; }
        [[nodiscard]] const VertexBuffer &GetVBO() const { return m_VBO; }
        [[nodiscard]] const IndexBuffer &GetIBO() const { return m_IBO; }

        [[nodiscard]] uint32_t GetIndexCount() const { return m_IBO.GetCount(); }

        std::string &GetFactoryId() { return m_FactoryId; }
        void SetFactoryId(const std::string &facid) { m_FactoryId = facid; }

    private:
        std::string m_FactoryId; // for serialization
        VertexArray m_VAO;
        VertexBuffer m_VBO;
        IndexBuffer m_IBO;
        MeshData m_TempData;      // during upload
        MeshData m_CustomDataBin; // temporary store untill serialization
        bool isCustom = false;
        float m_BoundingRadius = 0.0f;
    };
} // namespace Rendering
