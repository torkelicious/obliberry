

#ifndef OBLIBERRY_TEXTURE_H
#define OBLIBERRY_TEXTURE_H
#include <string>

#include "glad/glad.h"


class Texture {
public:
    // disable copying
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    // allow moving
    Texture(Texture&&) = default;
    Texture& operator=(Texture&&) = default;

    Texture(
        const std::string &path,
        GLuint minFilter = GL_NEAREST_MIPMAP_NEAREST,
        GLuint magFilter = GL_NEAREST,
        GLuint wrapS = GL_CLAMP_TO_EDGE,
        GLuint wrapT = GL_CLAMP_TO_EDGE
    );

    ~Texture();

    void Bind(unsigned int slot = 0) const;

    void Unbind() const;

    static Texture *White();

private:
    Texture() = default;

    GLuint m_ID;
    std::string m_FilePath;
    unsigned char *m_ImgLocBuffer;
    int m_Width, m_Height, m_BPP;
};


#endif //OBLIBERRY_TEXTURE_H
