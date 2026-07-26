#pragma once

#include <string>
#include <utility>
#include <vector>

#include "glad/glad.h"

namespace Rendering {
    class Texture {
    public:
        // disable copying
        Texture(const Texture &) = delete;

        Texture &operator=(const Texture &) = delete;

        // allow moving
        Texture(Texture &&other) noexcept : m_ID(other.m_ID), m_FilePath(std::move(other.m_FilePath)), m_PixelData(std::move(other.m_PixelData)),
                                            m_Width(other.m_Width), m_Height(other.m_Height), m_BPP(other.m_BPP),
                                            m_MinFilter(other.m_MinFilter), m_MagFilter(other.m_MagFilter), m_WrapS(other.m_WrapS), m_WrapT(other.m_WrapT),
                                            m_IsWhiteTexture(other.m_IsWhiteTexture) {
            other.m_ID = 0;
        }

        Texture &operator=(Texture &&other) noexcept {
            if (this != &other) {
                if (m_ID != 0)
                    glDeleteTextures(1, &m_ID);
                m_ID = other.m_ID;
                other.m_ID = 0;
                m_FilePath = std::move(other.m_FilePath);
                m_PixelData = std::move(other.m_PixelData);
                m_Width = other.m_Width;
                m_Height = other.m_Height;
                m_BPP = other.m_BPP;
                m_MinFilter = other.m_MinFilter;
                m_MagFilter = other.m_MagFilter;
                m_WrapS = other.m_WrapS;
                m_WrapT = other.m_WrapT;
                m_IsWhiteTexture = other.m_IsWhiteTexture;
            }
            return *this;
        }

        // file path constructor
        explicit Texture(std::string path, GLuint minFilter = GL_NEAREST_MIPMAP_NEAREST, GLuint magFilter = GL_NEAREST, GLuint wrapS = GL_CLAMP_TO_EDGE, GLuint wrapT = GL_CLAMP_TO_EDGE);

        // raw data
        Texture(int width, int height, const unsigned char *data, GLuint minFilter = GL_NEAREST, GLuint magFilter = GL_NEAREST, GLuint wrapS = GL_CLAMP_TO_EDGE, GLuint wrapT = GL_CLAMP_TO_EDGE);

        ~Texture();

        void InitGL();

        void Bind(unsigned int slot = 0) const;

        static void Unbind();

        void UpdateData(const unsigned char *data, int width, int height);

        [[nodiscard]] std::string &GetPath() { return m_FilePath; }
        [[nodiscard]] const std::string &GetPath() const { return m_FilePath; }

        [[nodiscard]] int GetWidth() const { return m_Width; }
        [[nodiscard]] int GetHeight() const { return m_Height; }

        [[nodiscard]] GLuint GetID() const { return m_ID; }

        static Texture *White();

    private:
        Texture() = default;

        GLuint m_ID = 0;
        std::string m_FilePath;

        std::vector<unsigned char> m_PixelData;

        int m_Width = 0, m_Height = 0, m_BPP = 0;

        GLuint m_MinFilter = GL_NEAREST_MIPMAP_NEAREST;
        GLuint m_MagFilter = GL_NEAREST;
        GLuint m_WrapS = GL_CLAMP_TO_EDGE;
        GLuint m_WrapT = GL_CLAMP_TO_EDGE;

        bool m_IsWhiteTexture = false;
    };
} // namespace Rendering
