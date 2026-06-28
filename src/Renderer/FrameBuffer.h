#pragma once
#include <cstdint>
#include <glad/glad.h>
#include <iostream>

class FrameBuffer {
public:
    FrameBuffer(const uint32_t width, const uint32_t height) { Invalidate(width, height); }

    ~FrameBuffer() {
        if (m_RendererID) {
            glDeleteFramebuffers(1, &m_RendererID);
            glDeleteTextures(1, &m_ColorAtt);
            glDeleteTextures(1, &m_EntityIDAtt);
        }
    }

    [[nodiscard]] uint32_t GetWidth() const { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const { return m_Height; }

    void Invalidate(const uint32_t width, const uint32_t height) {
        if (m_RendererID) {
            glDeleteFramebuffers(1, &m_RendererID);
            glDeleteTextures(1, &m_ColorAtt);
            glDeleteTextures(1, &m_EntityIDAtt);
        }
        m_Width = width;
        m_Height = height;

        glGenFramebuffers(1, &m_RendererID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

        // Color Attachment
        glGenTextures(1, &m_ColorAtt);
        glBindTexture(GL_TEXTURE_2D, m_ColorAtt);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAtt, 0);

        // Entity ID
        glGenTextures(1, &m_EntityIDAtt);
        glBindTexture(GL_TEXTURE_2D, m_EntityIDAtt);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, m_Width, m_Height, 0, GL_RED_INTEGER, GL_INT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_EntityIDAtt, 0);

        const GLenum buffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, buffers);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Error: Framebuffer is incomplete!\n";
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
        glViewport(0, 0, m_Width, m_Height);
    }

    void Unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // EDITOR PICKING

    // call after clearing
    void ClearEntityIDAttachment(const int clearValue = -1) const {
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
        // Clears the specific color buffer index
        glClearBufferiv(GL_COLOR, 1, &clearValue);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // on click
    int ReadEntityID(const uint32_t x, const uint32_t y) const {
        if (x >= m_Width || y >= m_Height)
            return -1;

        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
        glReadBuffer(GL_COLOR_ATTACHMENT1);

        int pixelData = -1;
        glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);

        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return pixelData;
    }

    uint32_t GetColorAttID() const { return m_ColorAtt; }
    uint32_t GetEntityIDAttID() const { return m_EntityIDAtt; }

private:
    uint32_t m_RendererID = 0;
    uint32_t m_ColorAtt = 0;
    uint32_t m_EntityIDAtt = 0;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
};
