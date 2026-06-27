#pragma once
#include <cstdint>
#include <glad/glad.h>
#include <iostream>

class FrameBuffer {
public:
    FrameBuffer(uint32_t width, uint32_t height) { Invalidate(width, height); }

    ~FrameBuffer() {
        if (m_RendererID) {
            glDeleteFramebuffers(1, &m_RendererID);
            glDeleteTextures(1, &m_ColorAtt);
            glDeleteTextures(1, &m_DepthAtt);
        }
    }

    void Invalidate(uint32_t width, uint32_t height) {
        if (m_RendererID) {
            glDeleteFramebuffers(1, &m_RendererID);
            glDeleteTextures(1, &m_ColorAtt);
            glDeleteTextures(1, &m_DepthAtt);
        }
        m_Width = width;
        m_Height = height;

        glGenFramebuffers(1, &m_RendererID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

        glGenTextures(1, &m_ColorAtt);
        glBindTexture(GL_TEXTURE_2D, m_ColorAtt);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAtt, 0);

        glGenTextures(1, &m_DepthAtt);
        glBindTexture(GL_TEXTURE_2D, m_DepthAtt);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_Width, m_Height, 0, GL_DEPTH_STENCIL,
                     GL_UNSIGNED_INT_24_8, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAtt, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Error: Framebuffer is incomplete!\n";
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
        glViewport(0, 0, m_Width, m_Height);
    }

    void Unbind() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    uint32_t GetColorAttID() const { return m_ColorAtt; }

private:
    uint32_t m_RendererID = 0;
    uint32_t m_ColorAtt = 0;
    uint32_t m_DepthAtt = 0;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
};
