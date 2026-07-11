#pragma once

#include <string>
#include <vector>

#include "glad/glad.h"

namespace Rendering {
    class Texture {
    public:
        // disable copying
        Texture(const Texture &) = delete;

        Texture &operator=(const Texture &) = delete;

        // allow moving
        Texture(Texture &&) = default;

        Texture &operator=(Texture &&) = default;

        // file path constructor
        explicit Texture(const std::string path, GLuint minFilter = GL_NEAREST_MIPMAP_NEAREST, GLuint magFilter = GL_NEAREST, GLuint wrapS = GL_CLAMP_TO_EDGE, GLuint wrapT = GL_CLAMP_TO_EDGE);

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
