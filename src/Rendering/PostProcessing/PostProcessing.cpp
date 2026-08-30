#include "PostProcessing.h"
#include "Logger/LoggerService.h"
#include "Rendering/Types/Shader/Shader.h"
#include <chrono>
#include <glad/glad.h>
#include <optional>
#include <string>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "PostProcessing"

namespace Rendering::PostProcessing {

    void DrawFullscreenTriangle() {
        static GLuint s_EmptyVAO = 0;
        if (s_EmptyVAO == 0) {
            glGenVertexArrays(1, &s_EmptyVAO);
        }
        glBindVertexArray(s_EmptyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }

    static float GetEngineTime() {
        static const auto start = std::chrono::steady_clock::now();
        return std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
    }

    static void ApplyUniformValue(Shader &shader, const char *name, const UniformValue &value) {
        std::visit(
                [&](const auto &v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, float>)
                        shader.SetUniform1f(name, v);
                    else if constexpr (std::is_same_v<T, int>)
                        shader.SetUniform1i(name, v);
                    else if constexpr (std::is_same_v<T, glm::vec2>)
                        shader.SetUniformVec2(name, v);
                    else if constexpr (std::is_same_v<T, glm::vec3>)
                        shader.SetUniformVec3(name, v);
                    else if constexpr (std::is_same_v<T, glm::vec4>)
                        shader.SetUniformVec4(name, v);
                },
                value);
    }

    void PostEffect::ResolveShader() {
        shader = Core::ResourceManager::GetInstance().Get<Shader>(shaderKey);
        if (!shader)
            LOG_WARN(LOG_WHO, "PostEffect shader '" + shaderKey + "' not found in resource manager");
    }

    void PostProcessor::AddEffect(PostEffect fx) {
        if (!fx.shader)
            fx.ResolveShader();
        m_Effects.push_back(std::move(fx));
    }

    FrameBuffer *PostProcessor::Execute(FrameBuffer *scene, FrameBuffer *pingA, FrameBuffer *pingB) {
        FrameBuffer *ping[2] = {pingA, pingB};
        FrameBuffer *currentInput = scene;
        FrameBuffer *lastOutput = nullptr;
        int writeIdx = 0;
        bool ran = false;

        glDisable(GL_BLEND);

        for (auto &fx : m_Effects) {
            if (!fx.enabled || !fx.shader || !fx.shader->IsValid())
                continue;

            FrameBuffer *target = ping[writeIdx];
            target->Bind();
            glDrawBuffer(GL_COLOR_ATTACHMENT0);

            Shader &shader = *fx.shader;
            shader.Bind();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, currentInput->GetColorAttID());

            // engine uniforms
            shader.SetUniform1i("u_Texture", 0);
            shader.SetUniformVec2(kUniformResolution, {static_cast<float>(target->GetWidth()), static_cast<float>(target->GetHeight())});
            shader.SetUniform1f(kUniformTime, GetEngineTime());

            // serialized uniform bag
            for (const auto &[name, value] : fx.uniforms)
                ApplyUniformValue(shader, name.c_str(), value);

            DrawFullscreenTriangle();

            currentInput = target;
            lastOutput = target;
            writeIdx ^= 1;
            ran = true;
        }

        glEnable(GL_BLEND);
        return ran ? lastOutput : nullptr;
    }


    // serialization is not really wired in yet, im just drafting the architechture

    // serialization
    static nlohmann::json UniformValueToJson(const UniformValue &v) {
        return std::visit(
                []<typename T0>(const T0 &val) -> nlohmann::json {
                    using T = std::decay_t<T0>;
                    if constexpr (std::is_same_v<T, float>)
                        return val;
                    else if constexpr (std::is_same_v<T, int>)
                        return val;
                    else if constexpr (std::is_same_v<T, glm::vec2>)
                        return {val.x, val.y};
                    else if constexpr (std::is_same_v<T, glm::vec3>)
                        return {val.x, val.y, val.z};
                    else
                        return {val.x, val.y, val.z, val.w};
                },
                v);
    }

    static std::optional<UniformValue> UniformValueFromJson(const nlohmann::json &j) {
        if (j.is_number_integer())
            return j.get<int>();
        if (j.is_number())
            return j.get<float>();
        if (j.is_array()) {
            switch (j.size()) {
                case 2:
                    return glm::vec2{j[0].get<float>(), j[1].get<float>()};
                case 3:
                    return glm::vec3{j[0].get<float>(), j[1].get<float>(), j[2].get<float>()};
                case 4:
                    return glm::vec4{j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()};
                default:
                    break;
            }
        }
        return std::nullopt;
    }

    nlohmann::json PostProcessor::Serialize() const {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto &fx : m_Effects) {
            nlohmann::json uniforms = nlohmann::json::object();
            for (const auto &[name, value] : fx.uniforms)
                uniforms[name] = UniformValueToJson(value);

            arr.push_back({{"shader", fx.shaderKey}, {"enabled", fx.enabled}, {"uniforms", std::move(uniforms)}});
        }
        return arr;
    }

    bool PostProcessor::Deserialize(const nlohmann::json &j) {
        if (!j.is_array()) {
            LOG_ERROR(LOG_WHO, "PostEffect chain must be a JSON array");
            return false;
        }

        m_Effects.clear();

        for (const auto &entry : j) {
            PostEffect fx;

            try {
                if (entry.contains("shader"))
                    fx.shaderKey = entry["shader"].get<std::string>();
                if (entry.contains("enabled"))
                    fx.enabled = entry["enabled"].get<bool>();

                if (entry.contains("uniforms") && entry["uniforms"].is_object()) {
                    for (const auto &[name, value] : entry["uniforms"].items()) {
                        if (auto parsed = UniformValueFromJson(value)) {
                            fx.uniforms[name] = std::move(*parsed);
                        } else {
                            LOG_WARN(LOG_WHO, "Skipping uniform '" + name + "' on '" + fx.shaderKey + "': unsupported value type");
                        }
                    }
                }
            } catch (const std::exception &e) {
                LOG_ERROR(LOG_WHO, "Skipping malformed post effect entry: " + std::string(e.what()));
                continue;
            }
            if (fx.shaderKey.empty()) {
                LOG_WARN(LOG_WHO, "Skipping post effect without a shader key");
                continue;
            }
            fx.ResolveShader();
            m_Effects.push_back(std::move(fx));
        }

        return true;
    }

} // namespace Rendering::PostProcessing

#pragma pop_macro("LOG_WHO")
