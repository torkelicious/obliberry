#include "Texture.h"
#include <iostream>
#include <stb_image.h>

Texture::Texture(
    const std::string &path,
    const GLuint minFilter,
    const GLuint magFilter,
    const GLuint wrapS,
    const GLuint wrapT
)
    : m_ID(0), m_FilePath(path), m_ImgLocBuffer(nullptr), m_Width(0), m_Height(0), m_BPP(0) {
    std::cout << "Loading: " << path << "\n";
    stbi_set_flip_vertically_on_load(1);

    m_ImgLocBuffer = stbi_load(path.c_str(), &m_Width, &m_Height, &m_BPP, 4);

    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_2D, m_ID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

    if (m_ImgLocBuffer) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     m_Width, m_Height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE,
                     m_ImgLocBuffer);

        const bool usesMipmaps =
                minFilter == GL_NEAREST_MIPMAP_NEAREST ||
                minFilter == GL_LINEAR_MIPMAP_NEAREST ||
                minFilter == GL_NEAREST_MIPMAP_LINEAR ||
                minFilter == GL_LINEAR_MIPMAP_LINEAR;

        if (usesMipmaps)
            glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }

    stbi_image_free(m_ImgLocBuffer);
}

Texture::~Texture() {
    Unbind();
    if (m_ID != 0) {
        glDeleteTextures(1, &m_ID);
        m_ID = 0;
    }
}

void Texture::Bind(const unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_ID);
}

void Texture::Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture *Texture::White() {
    static Texture *instance = nullptr;

    if (!instance) {
        instance = new Texture();

        glGenTextures(1, &instance->m_ID);
        glBindTexture(GL_TEXTURE_2D, instance->m_ID);

        const uint32_t white = 0xFFFFFFFF;

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            1, 1,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            &white
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    return instance;
}
