#include "Shader.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include "Logger/LoggerService.h"
#include "IO/VFS/VFS.h"

#include "Rendering/InternalShaders.h"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "Shader"

namespace Rendering {
    Shader::Shader(const std::string &vertPath, const std::string &fragPath) : m_vertPath(vertPath), m_fragPath(fragPath) {
        m_VertexSrc = LoadFile(vertPath);
        m_FragmentSrc = LoadFile(fragPath);
    }

    Shader::Shader(std::string vertSrc, std::string fragSrc, std::string debugName)
        : m_vertPath(std::move(debugName)), m_fragPath(std::move(debugName)), m_VertexSrc(std::move(vertSrc)), m_FragmentSrc(std::move(fragSrc)) {
    }

    Shader::~Shader() {
        if (m_ID != 0) {
            glDeleteProgram(m_ID);
            m_ID = 0;
        }
    }

    void Shader::InitGL() {
        if (m_ID != 0)
            return;

        const GLuint vert = Compile(GL_VERTEX_SHADER, m_VertexSrc);
        const GLuint frag = Compile(GL_FRAGMENT_SHADER, m_FragmentSrc);
        m_ID = Link(vert, frag);

        if (m_ID == 0) {
            LOG_ERROR(LOG_WHO, "Failed to create program from:\n  " + m_vertPath + "\n  " + m_fragPath);
        }

        m_VertexSrc.clear();
        m_FragmentSrc.clear();
        m_VertexSrc.shrink_to_fit();
        m_FragmentSrc.shrink_to_fit();
    }

    void Shader::Reload() {
        if (m_ID != 0) {
            glDeleteProgram(m_ID);
            m_ID = 0;
        }
        m_UniformCache.clear();
        m_VertexSrc = LoadFile(m_vertPath);
        m_FragmentSrc = LoadFile(m_fragPath);
        InitGL();
    }


    void Shader::Bind() const {
        if (m_ID == 0)
            return;
        glUseProgram(m_ID);
    }

    void Shader::Unbind() {
        glUseProgram(0);
    }


    void Shader::SetUniform1i(const char *name, const int value) {
        const GLint loc = GetUniformLocation(name);
        if (loc == -1)
            return;
        glUniform1i(loc, value);
    }

    void Shader::SetUniform1f(const char *name, const float value) {
        const GLint loc = GetUniformLocation(name);
        if (loc == -1)
            return;
        glUniform1f(loc, value);
    }

    void Shader::SetUniformVec2(const char *name, const glm::vec2 &v) {
        const GLint loc = GetUniformLocation(name);
        if (loc == -1)
            return;
        glUniform2f(loc, v.x, v.y);
    }

    void Shader::SetUniformVec3(const char *name, const glm::vec3 &v) {
        const GLint loc = GetUniformLocation(name);
        if (loc == -1)
            return;
        glUniform3f(loc, v.x, v.y, v.z);
    }

    void Shader::SetUniformVec4(const char *name, const glm::vec4 &v) {
        const GLint loc = GetUniformLocation(name);
        if (loc == -1)
            return;
        glUniform4f(loc, v.x, v.y, v.z, v.w);
    }

    void Shader::SetUniformMat4(const char *name, const glm::mat4 &mat) {
        const GLint loc = GetUniformLocation(name);
        if (loc == -1)
            return;
        glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
    }

    GLint Shader::GetUniformLocation(const char *name) {
        if (m_ID == 0)
            return -1;

        for (const auto &[uniformName, loc] : m_UniformCache) {
            if (uniformName == name)
                return loc;
        }

        GLint location = glGetUniformLocation(m_ID, name);
        m_UniformCache.emplace_back(name, location);
        return location;
    }

    std::string Shader::LoadFile(const std::string &virtualPath) {
        std::optional<std::string> shaderSource = IO::VFS::ReadVirtual(virtualPath);
        if (!shaderSource.has_value()) {
            LOG_ERROR(LOG_WHO, "Failed to open shader through VFS: " + virtualPath);
            return {};
        }
        return std::move(shaderSource.value());
    }


    GLuint Shader::Compile(const GLenum type, const std::string &src) {
        if (src.empty()) {
            // Prevent confusing GLSL errors if file load failed.
            return 0;
        }

        const GLuint shader = glCreateShader(type);
        const char *cstr = src.c_str();
        glShaderSource(shader, 1, &cstr, nullptr);
        glCompileShader(shader);

        GLint ok = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[2048];
            glGetShaderInfoLog(shader, 2048, nullptr, log);
            LOG_ERROR(LOG_WHO, "Compile error:\n" + std::string(log));
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    GLuint Shader::Link(const GLuint vert, const GLuint frag) {
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
            LOG_ERROR(LOG_WHO, "Link error:\n" + std::string(log));
            glDeleteProgram(program);
            program = 0;
        }

        glDeleteShader(vert);
        glDeleteShader(frag);

        return program;
    }

} // namespace Rendering
#pragma pop_macro("LOG_WHO")
