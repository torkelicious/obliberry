#include "Texture.h"
#include <utility>
#include <stb_image.h>
#include "Core/LoggerService.h"
#include "IO/VFS.h"

constexpr auto LOG_WHO = "Texture";

namespace Rendering {
    Texture::Texture(std::string path, const GLuint minFilter, const GLuint magFilter, const GLuint wrapS,
                     const GLuint wrapT)
        : m_FilePath(std::move(path)), m_MinFilter(minFilter), m_MagFilter(magFilter), m_WrapS(wrapS), m_WrapT(wrapT) {
        LOG_INFO(LOG_WHO, "Loading: " + m_FilePath);
        stbi_set_flip_vertically_on_load(1);

        if (const std::optional<std::string> fileData = IO::VFS::ReadVirtual(m_FilePath)) {
            const auto *buffer = reinterpret_cast<const stbi_uc *>(fileData->data());
            const int len = static_cast<int>(fileData->size());

            if (unsigned char *loaded = stbi_load_from_memory(buffer, len, &m_Width, &m_Height, &m_BPP, 4)) {
                const auto size = static_cast<size_t>(m_Width * m_Height * 4);
                m_PixelData.assign(loaded, loaded + size);
                stbi_image_free(loaded);
            } else {
                LOG_ERROR(LOG_WHO, "Failed to decode image data for: " + m_FilePath);
            }
        } else {
            LOG_ERROR(LOG_WHO, "VFS could not find or read file: " + m_FilePath);
        }
    }

    Texture::Texture(const int width, const int height, const unsigned char *data, const GLuint minFilter,
                     const GLuint magFilter, const GLuint wrapS, const GLuint wrapT)
        : m_Width(width), m_Height(height), m_BPP(4), m_MinFilter(minFilter), m_MagFilter(magFilter), m_WrapS(wrapS),
          m_WrapT(wrapT) {
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
        if (m_ID != 0)
            return;

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
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         m_PixelData.data());

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

    void Texture::Unbind() { glBindTexture(GL_TEXTURE_2D, 0); }

    void Texture::UpdateData(const unsigned char *data, const int width, const int height) {
        Bind();
        if (width == m_Width && height == m_Height) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        } else {
            m_Width = width;
            m_Height = height;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
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
} // namespace Rendering
