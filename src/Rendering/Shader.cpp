#include "Shader.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include "Core/LoggerService.h"
#include "IO/VFS.h"


constexpr auto LOG_WHO = "Shader";

namespace Rendering {
    Shader::Shader(const std::string &vertPath, const std::string &fragPath) : m_vertPath(vertPath), m_fragPath(fragPath) {
        m_VertexSrc = LoadFile(vertPath);
        m_FragmentSrc = LoadFile(fragPath);
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

    void Shader::Unbind() { glUseProgram(0); }


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

    // FIXME/HACK: this is a bit of a hack, i should probably keep default fallbackshaders in internal/ build dir?, not
    // sure for now
    Shader *Shader::Default() {
        static Shader *instance = [] {
            // Hardcoded fallback
            auto *shader = new Shader("", "");
            shader->m_VertexSrc = R"(
#version 330 core
layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec2 a_UV;
layout(location = 2) in mat4 a_InstanceMatrix;
layout(location = 6) in int a_EntityID;
uniform mat4 u_VP;
uniform vec2 u_MapSize;
uniform vec2 u_MapOffset;
out vec2 v_UV;
out vec2 v_LightUV;
flat out int v_EntityID;
void main() {
    v_UV = a_UV;
    vec4 worldPos = a_InstanceMatrix * vec4(a_Pos, 1.0);
    v_LightUV = (worldPos.xy - u_MapOffset) / u_MapSize;
    gl_Position = u_VP * worldPos;
    v_EntityID = a_EntityID;
}
)";
            shader->m_FragmentSrc = R"(
#version 330 core
in vec2 v_UV;
in vec2 v_LightUV;
uniform sampler2D u_Texture;
uniform sampler2D u_LightTexture;
uniform vec4 u_Color;
uniform float u_Ambient;
flat in int v_EntityID;
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int OutEntityID;
void main() {
    vec4 tex = texture(u_Texture, v_UV);
    float finalAlpha = tex.a * u_Color.a;
    if (finalAlpha < 0.01) { discard; }
    vec3 light = texture(u_LightTexture, v_LightUV).rgb;
    light = max(light, vec3(u_Ambient));
    vec3 finalColor = tex.rgb * u_Color.rgb * light;
    FragColor = vec4(finalColor, finalAlpha);
    OutEntityID = v_EntityID;
}
)";
            shader->InitGL();
            return shader;
        }();
        return instance;
    }
} // namespace Rendering
