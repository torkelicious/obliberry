#include "UIRenderer.h"
#include "InternalUIShaders.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace UI {

    void UIRenderer::InitGL() {
        std::vector<unsigned int> indices;
        indices.reserve(MAX_QUADS * 6);
        for (unsigned int i = 0; i < MAX_QUADS; i++) {
            const unsigned int base = i * 4;
            indices.push_back(base + 0);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            indices.push_back(base + 0);
            indices.push_back(base + 2);
            indices.push_back(base + 3);
        }
        m_IBO = std::make_unique<Rendering::IndexBuffer>();
        m_IBO->Init(indices.data(), static_cast<unsigned int>(indices.size()));

        m_VBO = std::make_unique<Rendering::VertexBuffer>();
        m_VBO->Init(nullptr, MAX_QUADS * 4 * sizeof(UIVertex), GL_DYNAMIC_DRAW);

        // Vertex array
        m_VAO = std::make_unique<Rendering::VertexArray>();
        m_VAO->Init();
        m_VAO->Bind();
        Rendering::VertexBufferLayout layout;
        layout.Push(GL_FLOAT, 2); // position
        layout.Push(GL_FLOAT, 2); // UV
        layout.Push(GL_FLOAT, 4); // color
        m_VAO->AddBuffer(*m_VBO, layout);
        m_VAO->SetIndexBuffer(*m_IBO);
        glBindVertexArray(0);

        m_Shader = std::make_shared<Rendering::Shader>(kUIVertShader, kUIFragShader, "[Engine UI] UIShader");
        m_Shader->InitGL();

        Rendering::Texture::White();

        for (auto &buf : m_Vertices)
            buf.reserve(MAX_QUADS * 4);
        for (auto &buf : m_QuadTextures)
            buf.reserve(MAX_QUADS);

        m_GLInitialized = true;
    }

    void UIRenderer::BeginFrame(const uint32_t viewWidth, const uint32_t viewHeight) {
        m_Projection[m_SubmitIndex] = glm::ortho(0.0f, static_cast<float>(viewWidth), static_cast<float>(viewHeight), 0.0f, -1.0f, 1.0f);
        m_Vertices[m_SubmitIndex].clear();
        m_QuadTextures[m_SubmitIndex].clear();
    }

    void UIRenderer::SubmitQuad(const glm::vec2 pos, const glm::vec2 size, const glm::vec2 uvMin, const glm::vec2 uvMax, const Rendering::Texture *texture, const glm::vec4 color) {

        auto &verts = m_Vertices[m_SubmitIndex];
        // V is flipped
        verts.push_back({pos, glm::vec2(uvMin.x, uvMax.y), color});
        verts.push_back({pos + glm::vec2(size.x, 0.0f), glm::vec2(uvMax.x, uvMax.y), color});
        verts.push_back({pos + size, glm::vec2(uvMax.x, uvMin.y), color});
        verts.push_back({pos + glm::vec2(0.0f, size.y), glm::vec2(uvMin.x, uvMin.y), color});

        m_QuadTextures[m_SubmitIndex].push_back(texture);
    }

    void UIRenderer::SubmitRect(const glm::vec2 pos, const glm::vec2 size, const glm::vec4 color) { SubmitQuad(pos, size, {0.0f, 0.0f}, {1.0f, 1.0f}, Rendering::Texture::White(), color); }

    void UIRenderer::Flush() {
        const auto &verts = m_Vertices[m_RenderIndex];
        const auto &texs = m_QuadTextures[m_RenderIndex];

        if (verts.empty())
            return;

        // Build batches
        m_Batches.clear();
        const Rendering::Texture *currentTex = texs[0];
        uint32_t batchStart = 0;

        for (uint32_t i = 1; i < static_cast<uint32_t>(texs.size()); i++) {
            if (texs[i] != currentTex) {
                m_Batches.push_back({currentTex, batchStart * 6, (i - batchStart) * 6});
                currentTex = texs[i];
                batchStart = i;
            }
        }
        m_Batches.push_back({currentTex, batchStart * 6, (static_cast<uint32_t>(texs.size()) - batchStart) * 6});

        // Upload to GPU
        m_VBO->SetDataOrphaned(verts.data(), static_cast<unsigned int>(verts.size() * sizeof(UIVertex)));

        // Draw
        m_VAO->Bind();
        m_Shader->Bind();
        m_Shader->SetUniformMat4("u_Projection", m_Projection[m_RenderIndex]);

        for (const auto &[texture, indexOffset, indexCount] : m_Batches) {
            if (texture) {
                texture->Bind(0);
            }
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, reinterpret_cast<const void *>(static_cast<uintptr_t>(indexOffset * sizeof(unsigned int))));
        }

        m_Shader->Unbind();
        Rendering::VertexArray::Unbind();
    }

    void UIRenderer::SwapBuffers() { std::swap(m_SubmitIndex, m_RenderIndex); }

} // namespace UI
