#include "Texture.h"
#include <iostream>
#include <stb_image.h>

#include "IO/VFS.h"

Texture::Texture(
    const std::string &path,
    const GLuint minFilter,
    const GLuint magFilter,
    const GLuint wrapS,
    const GLuint wrapT
)
    : m_FilePath(path),
      m_MinFilter(minFilter), m_MagFilter(magFilter), m_WrapS(wrapS), m_WrapT(wrapT) {
    const std::filesystem::path absolutePath = IO::VFS::Resolve(m_FilePath);
    const std::string ospath = absolutePath.string();

    std::cout << "Loading: " << ospath << "\n";
    stbi_set_flip_vertically_on_load(1);

    if (unsigned char *loaded = stbi_load(ospath.c_str(), &m_Width, &m_Height, &m_BPP, 4)) {
        const auto size = static_cast<size_t>(m_Width * m_Height * 4);
        m_PixelData.assign(loaded, loaded + size);
        stbi_image_free(loaded);
    }
}

Texture::Texture(
    const int width,
    const int height,
    const unsigned char *data,
    const GLuint minFilter,
    const GLuint magFilter,
    const GLuint wrapS,
    const GLuint wrapT
)
    : m_Width(width), m_Height(height), m_BPP(4),
      m_MinFilter(minFilter), m_MagFilter(magFilter), m_WrapS(wrapS), m_WrapT(wrapT) {
    if (data) {
        const auto size = static_cast<size_t>(width * height * 4); // RGBA
        m_PixelData.assign(data, data + size);
    }
}

Texture::~Texture() {
    if (m_ID != 0) {
        glDeleteTextures(1, &m_ID);
        m_ID = 0;
    }
}

void Texture::InitGL() {
    if (m_ID != 0) return;

    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_2D, m_ID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_MinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_MagFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_WrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_WrapT);

    // white default texture
    if (m_IsWhiteTexture) {
        constexpr uint32_t white = 0xFFFFFFFF;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    // loaded from disk or dynamic data
    else if (!m_PixelData.empty()) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_PixelData.data());

        // mipmaps
        if (m_MinFilter == GL_NEAREST_MIPMAP_NEAREST || m_MinFilter == GL_LINEAR_MIPMAP_NEAREST ||
            m_MinFilter == GL_NEAREST_MIPMAP_LINEAR || m_MinFilter == GL_LINEAR_MIPMAP_LINEAR) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        m_PixelData.clear();
        m_PixelData.shrink_to_fit();
    }
    // if allocating size
    else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
}

void Texture::Bind(const unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_ID);
}

void Texture::Unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::UpdateData(const unsigned char *data, const int width, const int height) {
    Bind();
    if (width == m_Width && height == m_Height) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    } else {
        m_Width = width;
        m_Height = height;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     m_Width, m_Height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE,
                     data);
    }
}

Texture *Texture::White() {
    static Texture *instance = [] {
        auto *tex = new Texture();
        tex->m_IsWhiteTexture = true;
        tex->InitGL();
        return tex;
    }();
    return instance;
}
