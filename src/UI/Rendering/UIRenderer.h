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

    struct UIBatch {
        const Rendering::Texture *texture = nullptr;
        uint32_t indexOffset = 0; // byte offset into IBO
        uint32_t indexCount = 0;  // number of indices to draw
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

        // convenience
        void SubmitRect(const RectTransform &rect, const glm::vec4 color) { SubmitRect(rect.Position, rect.Scale, color); }

        void SubmitQuad(const RectTransform &rect, const glm::vec2 uvMin, const glm::vec2 uvMax, const Rendering::Texture *texture, const glm::vec4 color) {
            SubmitQuad(rect.Position, rect.Scale, uvMin, uvMax, texture, color);
        }

        // call on render thread.
        void Flush();

        void SwapBuffers();

    private:
        static constexpr uint32_t MAX_QUADS = 10000;

        size_t m_SubmitIndex = 0;
        size_t m_RenderIndex = 1;

        glm::mat4 m_Projection[2] = {glm::mat4(1.0f), glm::mat4(1.0f)};

        std::vector<UIVertex> m_Vertices[2];
        std::vector<UIBatch> m_Batches;
        std::vector<const Rendering::Texture *> m_QuadTextures[2];

        std::unique_ptr<Rendering::VertexBuffer> m_VBO;
        std::unique_ptr<Rendering::VertexArray> m_VAO;
        std::unique_ptr<Rendering::IndexBuffer> m_IBO;
        std::shared_ptr<Rendering::Shader> m_Shader;

        bool m_GLInitialized = false;
    };

} // namespace UI
