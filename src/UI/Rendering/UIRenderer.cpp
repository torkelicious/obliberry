#include "UIRenderer.h"
#include "InternalUIShaders.h"
#include "Rendering/Types/Shader/Shader.h"
#include "Rendering/Types/Texture/Texture.h"
#include "Logger/LoggerService.h"
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "UIRenderer"

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

        m_SDFShader = std::make_shared<Rendering::Shader>(kUIVertShader, kUISDFFragShader, "[Engine UI] UISDFShader");
        m_SDFShader->InitGL();

        Rendering::Texture::White();

        for (auto &buf : m_Vertices)
            buf.reserve(MAX_QUADS * 4);
        for (auto &buf : m_QuadTextures)
            buf.reserve(MAX_QUADS);

        m_GLInitialized = true;
    }

    void UIRenderer::BeginFrame(const uint32_t viewWidth, const uint32_t viewHeight) {
        m_ActualWindowSize[m_SubmitIndex] = {viewWidth, viewHeight};
        m_FrameGameResolution[m_SubmitIndex] = m_GameResolution;
        if (m_GameResolution.x > 0 && m_GameResolution.y > 0) {
            m_Projection[m_SubmitIndex] = glm::ortho(0.0f, m_GameResolution.x, m_GameResolution.y, 0.0f, -1.0f, 1.0f);
        } else {
            m_Projection[m_SubmitIndex] = glm::ortho(0.0f, static_cast<float>(viewWidth), static_cast<float>(viewHeight), 0.0f, -1.0f, 1.0f);
        }
        m_Vertices[m_SubmitIndex].clear();
        m_QuadTextures[m_SubmitIndex].clear();
        m_QuadShader[m_SubmitIndex].clear();
        m_QuadSDFScale[m_SubmitIndex].clear();
        m_QuadSDFSpread[m_SubmitIndex].clear();
        m_OverflowLogged = false;
    }

    // returns true if there is capacity for one more quad
    inline bool UIRenderer::HasQuadCapacity() {
        if (m_Vertices[m_SubmitIndex].size() / 4 < MAX_QUADS)
            return true;
        if (!m_OverflowLogged) {
            LOG_WARN(LOG_WHO, "UI quad capacity reached (" + std::to_string(MAX_QUADS) + "); dropping further submissions this frame");
            m_OverflowLogged = true;
        }
        return false;
    }

    void UIRenderer::SubmitQuad(const glm::vec2 pos, const glm::vec2 size, const glm::vec2 uvMin, const glm::vec2 uvMax, const std::shared_ptr<Rendering::Texture> &texture, const glm::vec4 color) {
        if (!HasQuadCapacity())
            return;

        auto &verts = m_Vertices[m_SubmitIndex];
        // V is flipped
        verts.push_back({.Position = pos, .UV = glm::vec2(uvMin.x, uvMax.y), .Color = color});
        verts.push_back({.Position = pos + glm::vec2(size.x, 0.0f), .UV = glm::vec2(uvMax.x, uvMax.y), .Color = color});
        verts.push_back({.Position = pos + size, .UV = glm::vec2(uvMax.x, uvMin.y), .Color = color});
        verts.push_back({.Position = pos + glm::vec2(0.0f, size.y), .UV = glm::vec2(uvMin.x, uvMin.y), .Color = color});

        m_QuadTextures[m_SubmitIndex].push_back(texture);
        m_QuadShader[m_SubmitIndex].push_back(BatchShader::REGULAR);
        m_QuadSDFScale[m_SubmitIndex].push_back(1.0f);
        m_QuadSDFSpread[m_SubmitIndex].push_back(8.0f);
    }

    void UIRenderer::SubmitRect(const glm::vec2 pos, const glm::vec2 size, const glm::vec4 color) { SubmitQuad(pos, size, {0.0f, 0.0f}, {1.0f, 1.0f}, nullptr, color); }

    void UIRenderer::SubmitSDFQuad(const glm::vec2 pos, const glm::vec2 size, const glm::vec2 uvMin, const glm::vec2 uvMax, const std::shared_ptr<Rendering::Texture> &texture, const glm::vec4 color, const float sdfScale,
            const float sdfSpread) {
        if (!HasQuadCapacity())
            return;

        auto &verts = m_Vertices[m_SubmitIndex];
        verts.push_back({.Position = pos, .UV = glm::vec2(uvMin.x, uvMax.y), .Color = color});
        verts.push_back({.Position = pos + glm::vec2(size.x, 0.0f), .UV = glm::vec2(uvMax.x, uvMax.y), .Color = color});
        verts.push_back({.Position = pos + size, .UV = glm::vec2(uvMax.x, uvMin.y), .Color = color});
        verts.push_back({.Position = pos + glm::vec2(0.0f, size.y), .UV = glm::vec2(uvMin.x, uvMin.y), .Color = color});

        m_QuadTextures[m_SubmitIndex].push_back(texture);
        m_QuadShader[m_SubmitIndex].push_back(BatchShader::SDF);
        m_QuadSDFScale[m_SubmitIndex].push_back(sdfScale);
        m_QuadSDFSpread[m_SubmitIndex].push_back(sdfSpread);
    }

    void UIRenderer::Flush(const size_t renderIndex, const uint32_t renderTargetWidth, const uint32_t renderTargetHeight) {
        auto &verts = m_Vertices[renderIndex];
        auto &texs = m_QuadTextures[renderIndex];

        if (verts.empty())
            return;

        const glm::uvec2 targetSize = renderTargetWidth > 0 && renderTargetHeight > 0 ? glm::uvec2{renderTargetWidth, renderTargetHeight} : m_ActualWindowSize[renderIndex];
        {
            std::lock_guard lock(m_RenderTargetMutex);
            m_LastRenderTargetSize = targetSize;
        }
        const glm::vec2 gameResolution = m_FrameGameResolution[renderIndex];

        if (gameResolution.x > 0 && gameResolution.y > 0 && targetSize.x > 0 && targetSize.y > 0) {
            const float gameAspect = gameResolution.x / gameResolution.y;
            const float targetAspect = static_cast<float>(targetSize.x) / static_cast<float>(targetSize.y);

            uint32_t vpX = 0, vpY = 0, vpW, vpH;
            if (targetAspect > gameAspect) {
                vpH = targetSize.y;
                vpW = static_cast<uint32_t>(static_cast<float>(vpH) * gameAspect);
                vpX = (targetSize.x - vpW) / 2;
                vpY = 0;
            } else {
                vpW = targetSize.x;
                vpH = static_cast<uint32_t>(static_cast<float>(vpW) / gameAspect);
                vpX = 0;
                vpY = (targetSize.y - vpH) / 2;
            }
            glViewport(static_cast<GLint>(vpX), static_cast<GLint>(targetSize.y - vpY - vpH), static_cast<GLsizei>(vpW), static_cast<GLsizei>(vpH));
            glEnable(GL_SCISSOR_TEST);
            glScissor(static_cast<GLint>(vpX), static_cast<GLint>(targetSize.y - vpY - vpH), static_cast<GLsizei>(vpW), static_cast<GLsizei>(vpH));
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        // Build batches
        m_Batches.clear();
        m_Batches.reserve(texs.size());
        const Rendering::Texture *currentTex = texs[0].get();
        BatchShader currentShader = m_QuadShader[renderIndex][0];
        float currentSDFScale = m_QuadSDFScale[renderIndex][0];
        float currentSDFSpread = m_QuadSDFSpread[renderIndex][0];
        uint32_t batchStart = 0;

        for (uint32_t i = 1; i < static_cast<uint32_t>(texs.size()); i++) {
            if (texs[i].get() != currentTex || m_QuadShader[renderIndex][i] != currentShader || m_QuadSDFScale[renderIndex][i] != currentSDFScale || m_QuadSDFSpread[renderIndex][i] != currentSDFSpread) {
                m_Batches.push_back({.texture = currentTex, .indexOffset = batchStart * 6, .indexCount = (i - batchStart) * 6, .shader = currentShader, .sdfScale = currentSDFScale, .sdfSpread = currentSDFSpread});
                currentTex = texs[i].get();
                currentShader = m_QuadShader[renderIndex][i];
                currentSDFScale = m_QuadSDFScale[renderIndex][i];
                currentSDFSpread = m_QuadSDFSpread[renderIndex][i];
                batchStart = i;
            }
        }
        m_Batches.push_back({.texture = currentTex,
                .indexOffset = batchStart * 6,
                .indexCount = (static_cast<uint32_t>(texs.size()) - batchStart) * 6,
                .shader = currentShader,
                .sdfScale = currentSDFScale,
                .sdfSpread = currentSDFSpread});

        // Upload to GPU
        m_VBO->SetDataOrphaned(verts.data(), static_cast<unsigned int>(verts.size() * sizeof(UIVertex)));

        // Draw
        m_VAO->Bind();
        m_Shader->Bind();
        m_Shader->SetUniformMat4("u_Projection", m_Projection[renderIndex]);
        m_SDFShader->Bind();
        m_SDFShader->SetUniformMat4("u_Projection", m_Projection[renderIndex]);

        for (const auto &batch : m_Batches) {
            if (batch.shader == BatchShader::SDF) {
                m_SDFShader->Bind();
                m_SDFShader->SetUniform1f("u_Spread", batch.sdfSpread);
                m_SDFShader->SetUniform1f("u_Scale", batch.sdfScale);
            } else {
                m_Shader->Bind();
            }
            const Rendering::Texture *texture = batch.texture ? batch.texture : Rendering::Texture::White();
            texture->Bind(0);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(batch.indexCount), GL_UNSIGNED_INT, reinterpret_cast<const void *>(batch.indexOffset * sizeof(unsigned int)));
        }

        m_Shader->Unbind();
        Rendering::VertexArray::Unbind();

        if (gameResolution.x > 0 && gameResolution.y > 0 && targetSize.x > 0 && targetSize.y > 0) {
            glDisable(GL_SCISSOR_TEST);
            glViewport(0, 0, static_cast<GLsizei>(targetSize.x), static_cast<GLsizei>(targetSize.y));
        }
        m_Batches.clear();
        verts.clear();
        texs.clear();
    }

    void UIRenderer::SwapBuffers() { m_SubmitIndex = (m_SubmitIndex + 1) % 2; }

    void UIRenderer::SetGameResolution(const uint32_t width, const uint32_t height) { m_GameResolution = {static_cast<float>(width), static_cast<float>(height)}; }

    glm::vec2 UIRenderer::WindowToGameCoords(const float winX, const float winY) const {
        // legacy
        // prefer the explicit overload when possible.
        glm::uvec2 targetSize;
        {
            std::lock_guard lock(m_RenderTargetMutex);
            targetSize = m_LastRenderTargetSize;
        }
        return WindowToGameCoords(winX, winY, static_cast<float>(targetSize.x), static_cast<float>(targetSize.y));
    }

    glm::vec2 UIRenderer::WindowToGameCoords(const float winX, const float winY, const float viewWidth, const float viewHeight) const {
        if (m_GameResolution.x <= 0 || m_GameResolution.y <= 0 || viewWidth <= 0.0f || viewHeight <= 0.0f)
            return {winX, winY};

        const float gameAspect = m_GameResolution.x / m_GameResolution.y;
        const float targetAspect = viewWidth / viewHeight;

        float vpX = 0.0f, vpY = 0.0f, vpW, vpH;
        if (targetAspect > gameAspect) {
            vpH = viewHeight;
            vpW = vpH * gameAspect;
            vpX = (viewWidth - vpW) * 0.5f;
        } else {
            vpW = viewWidth;
            vpH = vpW / gameAspect;
            vpY = (viewHeight - vpH) * 0.5f;
        }

        const float gameX = (winX - vpX) * (m_GameResolution.x / vpW);
        const float gameY = (winY - vpY) * (m_GameResolution.y / vpH);

        return {gameX, gameY};
    }

    void UI::UIRenderer::Shutdown() {
        m_Batches.clear();

        for (size_t i = 0; i < 2; ++i) {
            m_Vertices[i].clear();
            m_QuadTextures[i].clear();
            m_QuadShader[i].clear();
            m_QuadSDFScale[i].clear();
            m_QuadSDFSpread[i].clear();
        }

        m_VAO.reset();
        m_VBO.reset();
        m_IBO.reset();
        m_Shader.reset();
        m_SDFShader.reset();

        m_GLInitialized = false;
    }


} // namespace UI
#pragma pop_macro("LOG_WHO")
