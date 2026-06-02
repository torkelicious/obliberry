#ifndef ISOMETRICGAME_TEXTURE_H
#define ISOMETRICGAME_TEXTURE_H

#include <string>

#include "glad/glad.h"

class Texture {
public:
    Texture(
        const std::string &path,
        GLuint minFilter = GL_LINEAR,
        GLuint magFilter = GL_LINEAR,
        GLuint wrapS = GL_CLAMP_TO_EDGE,
        GLuint wrapT = GL_CLAMP_TO_EDGE
    );

    ~Texture();

    void Bind(unsigned int slot = 0) const;

    void Unbind() const;

private:
    GLuint m_ID;
    std::string m_FilePath;
    unsigned char *m_ImgLocBuffer;
    int m_Width, m_Height, m_BPP;
};


#endif //ISOMETRICGAME_TEXTURE_H
