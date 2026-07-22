#pragma once

#include "UI/RectTransform.h"

#include "Rendering/VertexBuffer.h"
#include "Rendering/VertexArray.h"
#include "Rendering/IndexBuffer.h"
#include "Rendering/VertexBufferLayout.h"

#include <glm/glm.hpp>
#include <cstdint>
#include <memory>
#include <vector>

namespace Rendering {
    class Texture;
    class Shader;
} // namespace Rendering

namespace UI {

    struct UIVertex {
        glm::vec2 Position;
        glm::vec2 UV;
        glm::vec4 Color;
    };

    enum class BatchShader : uint8_t { REGULAR, SDF };

    struct UIBatch {
        const Rendering::Texture *texture = nullptr;
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
        BatchShader shader = BatchShader::REGULAR;
        float sdfScale = 1.0f;  // only used for SDF
        float sdfSpread = 8.0f; // only used for SDF
    };

    class UIRenderer {
    public:
        UIRenderer() = default;
        ~UIRenderer() = default;

        UIRenderer(const UIRenderer &) = delete;
        UIRenderer &operator=(const UIRenderer &) = delete;
        UIRenderer(UIRenderer &&) = delete;
        UIRenderer &operator=(UIRenderer &&) = delete;

        void InitGL();

        void BeginFrame(uint32_t viewWidth, uint32_t viewHeight);

        //  textured quad
        void SubmitQuad(glm::vec2 pos, glm::vec2 size, glm::vec2 uvMin, glm::vec2 uvMax, const Rendering::Texture *texture, glm::vec4 color);

        // colored rectangle
        void SubmitRect(glm::vec2 pos, glm::vec2 size, glm::vec4 color);

        void SubmitSDFQuad(glm::vec2 pos, glm::vec2 size, glm::vec2 uvMin, glm::vec2 uvMax, const Rendering::Texture *texture, glm::vec4 color, float sdfScale, float sdfSpread = 8.0f);

        // convenience
        void SubmitRect(const RectTransform &rect, const glm::vec4 color) { SubmitRect(rect.Position, rect.Scale, color); }

        void SubmitQuad(const RectTransform &rect, const glm::vec2 uvMin, const glm::vec2 uvMax, const Rendering::Texture *texture, const glm::vec4 color) {
            SubmitQuad(rect.Position, rect.Scale, uvMin, uvMax, texture, color);
        }

        // call on render thread.
        void Flush(uint32_t renderTargetWidth = 0, uint32_t renderTargetHeight = 0);

        void SwapBuffers();

        void SetGameResolution(uint32_t width, uint32_t height);
        [[nodiscard]] glm::vec2 GetGameResolution() const { return m_GameResolution; }

        [[nodiscard]] glm::vec2 WindowToGameCoords(float winX, float winY) const;

    private:
        static constexpr uint32_t MAX_QUADS = 10000;

        size_t m_SubmitIndex = 0;
        size_t m_RenderIndex = 1;

        glm::mat4 m_Projection[2] = {glm::mat4(1.0f), glm::mat4(1.0f)};
        glm::vec2 m_GameResolution = {0.0f, 0.0f};
        glm::uvec2 m_ActualWindowSize = {0, 0};     // framebuffer/window size for letterboxing
        glm::uvec2 m_LastRenderTargetSize = {0, 0}; // render target used in last Flush

        std::vector<UIVertex> m_Vertices[2];
        std::vector<UIBatch> m_Batches;
        std::vector<const Rendering::Texture *> m_QuadTextures[2];
        std::vector<BatchShader> m_QuadShader[2];
        std::vector<float> m_QuadSDFScale[2];
        std::vector<float> m_QuadSDFSpread[2];

        std::unique_ptr<Rendering::VertexBuffer> m_VBO;
        std::unique_ptr<Rendering::VertexArray> m_VAO;
        std::unique_ptr<Rendering::IndexBuffer> m_IBO;
        std::shared_ptr<Rendering::Shader> m_Shader;
        std::shared_ptr<Rendering::Shader> m_SDFShader;

        bool m_GLInitialized = false;
    };

} // namespace UI
