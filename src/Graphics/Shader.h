#ifndef ISOMETRICGAME_SHADER_H
#define ISOMETRICGAME_SHADER_H
#include <string>
#include <glm/glm.hpp>
#include "glad/glad.h"
#include <unordered_map>


class Shader {
public:
    Shader(const std::string &vertexSrc, const std::string &fragmentSrc);

    ~Shader();

    void Bind() const;

    void Unbind() const;

    GLuint GetID() const { return m_ID; }

    void SetUniform1i(const std::string &name, const int value);

    void SetUniformMat4(const std::string &name, const glm::mat4 &mat);

    void SetUniformVec2(const std::string &name, const glm::vec2 &v);

private:
    GLuint m_ID;
    std::unordered_map<std::string, GLint> m_UniformCache;

    std::string LoadFile(const std::string &path);

    GLuint Compile(GLenum type, const std::string &src);

    GLuint Link(GLuint vert, GLuint frag);

    GLint GetUniformLocation(const std::string &name);
};


#endif //ISOMETRICGAME_SHADER_H
