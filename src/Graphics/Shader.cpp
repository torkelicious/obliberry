#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>


Shader::Shader(const std::string &vertPath, const std::string &fragPath) {
    std::string vertexSrc = LoadFile(vertPath);
    std::string fragmentSrc = LoadFile(fragPath);
    GLuint vert = Compile(GL_VERTEX_SHADER, vertexSrc);
    GLuint frag = Compile(GL_FRAGMENT_SHADER, fragmentSrc);
    m_ID = Link(vert, frag);
}

Shader::~Shader() {
}

void Shader::Bind() const {
    glUseProgram(m_ID);
}

void Shader::Unbind() const {
    glUseProgram(0);
}

void Shader::SetUniform1i(const std::string &name, const int value) {
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetUniformMat4(const std::string &name, const glm::mat4 &mat) {
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

void Shader::SetUniformVec2(const std::string &name, const glm::vec2 &v) {
    glUniform2f(GetUniformLocation(name), v.x, v.y);
}

void Shader::SetUniform1f(const std::string &name, float value) {
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetUniformVec4(const std::string &name, const glm::vec4 &v) {
    glUniform4f(GetUniformLocation(name), v.x, v.y, v.z, v.w);
}

std::string Shader::LoadFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "Failed to open shader: " << path << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint Shader::Compile(GLenum type, const std::string &src) {
    GLuint shader = glCreateShader(type);
    const char *cstr = src.c_str();
    glShaderSource(shader, 1, &cstr, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cout << "Shader compile error:\n" << log << std::endl;
        // maybe we should return something on failure but im not sure...
    }
    return shader;
}

GLuint Shader::Link(GLuint vert, GLuint frag) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    //glBindAttribLocation(program, 0, "a_Pos");
    //glBindAttribLocation(program, 1, "a_TexCoord");
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cout << "Shader link error:\n" << log << std::endl;
        // maybe we should return something on failure but im not sure...
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

GLint Shader::GetUniformLocation(const std::string &name) {
    if (m_UniformCache.find(name) != m_UniformCache.end())
        return m_UniformCache[name];

    GLint location = glGetUniformLocation(m_ID, name.c_str());
    m_UniformCache[name] = location;

    return location;
}
