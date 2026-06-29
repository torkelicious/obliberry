#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace Rendering {
    class Shader {
    public:
        // disable copying
        Shader(const Shader &) = delete;

        Shader &operator=(const Shader &) = delete;

        // allow moving
        Shader(Shader &&) = default;

        Shader &operator=(Shader &&) = default;

        Shader(const std::string &vertPath, const std::string &fragPath);

        ~Shader();

        void InitGL();

        bool IsValid() const { return m_ID != 0; }

        void Bind() const;

        static void Unbind();

        GLuint GetID() const { return m_ID; }

        void SetUniform1i(const char *name, int value);

        void SetUniform1f(const char *name, float value);

        void SetUniformVec2(const char *name, const glm::vec2 &v);

        void SetUniformVec4(const char *name, const glm::vec4 &v);

        void SetUniformMat4(const char *name, const glm::mat4 &mat);

        std::string &GetVertexPath() { return m_vertPath; }
        std::string &GetFragmentPath() { return m_fragPath; }

    private:
        std::string m_vertPath;
        std::string m_fragPath;

        // tmp
        std::string m_VertexSrc;
        std::string m_FragmentSrc;

        GLuint m_ID = 0;

        struct StringHash {
            using is_transparent = void;

            size_t operator()(const std::string_view sv) const noexcept {
                return std::hash<std::string_view>{}(sv);
            }
        };

        struct StringEqual {
            using is_transparent = void;

            bool operator()(const std::string_view a, const std::string_view b) const noexcept {
                return a == b;
            }
        };

        std::unordered_map<std::string, GLint, StringHash, StringEqual> m_UniformCache;

        static std::string LoadFile(const std::string &virtualPath);

        static GLuint Compile(GLenum type, const std::string &src);

        static GLuint Link(GLuint vert, GLuint frag);

        GLint GetUniformLocation(const char *name);
    };
} // namespace Rendering
