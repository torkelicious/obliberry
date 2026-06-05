#ifndef ISOMETRICGAME_SHADER_H
#define ISOMETRICGAME_SHADER_H

#include <string>
#include <unordered_map>

#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader {
public:
    Shader(const std::string &vertPath, const std::string &fragPath);

    ~Shader();

    bool IsValid() const { return m_ID != 0; }

    void Bind() const;

    void Unbind() const;

    GLuint GetID() const { return m_ID; }

    void SetUniform1i(const std::string &name, int value);

    void SetUniform1f(const std::string &name, float value);

    void SetUniformVec2(const std::string &name, const glm::vec2 &v);

    void SetUniformVec4(const std::string &name, const glm::vec4 &v);

    void SetUniformMat4(const std::string &name, const glm::mat4 &mat);

private:
    GLuint m_ID = 0;
    std::unordered_map<std::string, GLint> m_UniformCache;

    std::string LoadFile(const std::string &path);

    GLuint Compile(GLenum type, const std::string &src);

    GLuint Link(GLuint vert, GLuint frag);

    GLint GetUniformLocation(const std::string &name);

    bool EnsureBound() const;
};

#endif // ISOMETRICGAME_SHADER_H
