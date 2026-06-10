#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader(const std::string &vertPath, const std::string &fragPath)
    : m_vertPath(vertPath), m_fragPath(fragPath) // copied but its ok
{
    std::string vertexSrc = LoadFile(vertPath);
    std::string fragmentSrc = LoadFile(fragPath);

    GLuint vert = Compile(GL_VERTEX_SHADER, vertexSrc);
    GLuint frag = Compile(GL_FRAGMENT_SHADER, fragmentSrc);
    m_ID = Link(vert, frag);

    if (m_ID == 0) {
        std::cerr << "[Shader] Failed to create program from:\n  "
                << vertPath << "\n  " << fragPath << "\n";
    }
}

Shader::~Shader() {
    if (m_ID != 0) {
        glDeleteProgram(m_ID);
        m_ID = 0;
    }
}

void Shader::Bind() const {
    if (m_ID == 0) return;
    glUseProgram(m_ID);
}

void Shader::Unbind() const {
    glUseProgram(0);
}


void Shader::SetUniform1i(const std::string &name, int value) {
    GLint loc = GetUniformLocation(name);
    if (loc == -1) return;
    glUniform1i(loc, value);
}

void Shader::SetUniform1f(const std::string &name, float value) {
    GLint loc = GetUniformLocation(name);
    if (loc == -1) return;
    glUniform1f(loc, value);
}

void Shader::SetUniformVec2(const std::string &name, const glm::vec2 &v) {
    GLint loc = GetUniformLocation(name);
    if (loc == -1) return;
    glUniform2f(loc, v.x, v.y);
}

void Shader::SetUniformVec4(const std::string &name, const glm::vec4 &v) {
    GLint loc = GetUniformLocation(name);
    if (loc == -1) return;
    glUniform4f(loc, v.x, v.y, v.z, v.w);
}

void Shader::SetUniformMat4(const std::string &name, const glm::mat4 &mat) {
    GLint loc = GetUniformLocation(name);
    if (loc == -1) return;
    glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
}

GLint Shader::GetUniformLocation(const std::string &name) {
    if (m_ID == 0) return -1;

    auto it = m_UniformCache.find(name);
    if (it != m_UniformCache.end())
        return it->second;

    GLint location = glGetUniformLocation(m_ID, name.c_str());
    m_UniformCache.emplace(name, location);
    return location;
}

std::string Shader::LoadFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Shader] Failed to open shader: " << path << "\n";
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint Shader::Compile(GLenum type, const std::string &src) const {
    if (src.empty()) {
        // Prevent confusing GLSL errors if file load failed.
        return 0;
    }

    GLuint shader = glCreateShader(type);
    const char *cstr = src.c_str();
    glShaderSource(shader, 1, &cstr, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(shader, 2048, nullptr, log);
        std::cerr << "[Shader] Compile error:\n" << log << "\n";
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint Shader::Link(GLuint vert, GLuint frag) const {
    if (vert == 0 || frag == 0) {
        if (vert)
            glDeleteShader(vert);
        if (frag)
            glDeleteShader(frag);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(program, 2048, nullptr, log);
        std::cerr << "[Shader] Link error:\n" << log << "\n";
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    return program;
}
