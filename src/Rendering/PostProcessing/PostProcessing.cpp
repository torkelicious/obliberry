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

    static void ApplyUniformBag(Shader &shader, const std::unordered_map<std::string, UniformValue> &bag) {
        for (const auto &[name, value] : bag)
            ApplyUniformValue(shader, name.c_str(), value);
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

            Shader &shader = *fx.shader;
            shader.Bind();

            // composite passes sample the original scene on 1
            if (fx.wantsSceneTexture) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, scene->GetColorAttID());
                shader.SetUniform1i(kUniformScene, 1);
            }

            const int passCount = std::max(fx.passes, 1);

            for (int pass = 0; pass < passCount; ++pass) {
                FrameBuffer *target = ping[writeIdx];
                target->Bind();
                glDrawBuffer(GL_COLOR_ATTACHMENT0);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, currentInput->GetColorAttID());

                // engine uniforms
                // these will be skipped when the shader doesnt declare them
                const float w = static_cast<float>(target->GetWidth());
                const float h = static_cast<float>(target->GetHeight());
                shader.SetUniform1i("u_Texture", 0);
                shader.SetUniformVec2(kUniformResolution, {w, h});
                shader.SetUniformVec2(kUniformTexelSize, {1.0f / w, 1.0f / h});
                shader.SetUniform1f(kUniformTime, GetEngineTime());

                ApplyUniformBag(shader, fx.uniforms);
                if (pass < static_cast<int>(fx.passUniforms.size()))
                    ApplyUniformBag(shader, fx.passUniforms[pass]);

                DrawFullscreenTriangle();

                currentInput = target;
                lastOutput = target;
                writeIdx ^= 1;
                ran = true;
            }
        }

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
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
        if (j.is_boolean())
            return j.get<bool>() ? 1 : 0;
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

    static nlohmann::json UniformBagToJson(const std::unordered_map<std::string, UniformValue> &bag) {
        nlohmann::json obj = nlohmann::json::object();
        for (const auto &[name, value] : bag)
            obj[name] = UniformValueToJson(value);
        return obj;
    }

    static void UniformBagFromJson(const nlohmann::json &j, const std::string &shaderKey, std::unordered_map<std::string, UniformValue> &out) {
        if (!j.is_object())
            return;
        for (const auto &[name, value] : j.items()) {
            if (auto parsed = UniformValueFromJson(value)) {
                out[name] = std::move(*parsed);
            } else {
                LOG_WARN(LOG_WHO, "Skipping uniform '" + name + "' on '" + shaderKey + "': unsupported value type");
            }
        }
    }

    nlohmann::json PostProcessor::Serialize() const { return SerializeEffects(m_Effects); }

    nlohmann::json SerializeEffects(const std::vector<PostEffect> &effects) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto &fx : effects) {
            nlohmann::json entry = {{"shader", fx.shaderKey}, {"enabled", fx.enabled}};

            if (!fx.uniforms.empty())
                entry["uniforms"] = UniformBagToJson(fx.uniforms);
            if (fx.passes > 1)
                entry["passes"] = fx.passes;
            if (fx.wantsSceneTexture)
                entry["wantsSceneTexture"] = true;

            if (!fx.passUniforms.empty()) {
                nlohmann::json passArr = nlohmann::json::array();
                for (const auto &bag : fx.passUniforms)
                    passArr.push_back(UniformBagToJson(bag));
                entry["passUniforms"] = std::move(passArr);
            }

            arr.push_back(std::move(entry));
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
                if (entry.contains("passes"))
                    fx.passes = std::max(entry["passes"].get<int>(), 1);
                if (entry.contains("wantsSceneTexture"))
                    fx.wantsSceneTexture = entry["wantsSceneTexture"].get<bool>();

                if (entry.contains("uniforms"))
                    UniformBagFromJson(entry["uniforms"], fx.shaderKey, fx.uniforms);

                if (entry.contains("passUniforms") && entry["passUniforms"].is_array()) {
                    for (const auto &passBag : entry["passUniforms"]) {
                        std::unordered_map<std::string, UniformValue> bag;
                        UniformBagFromJson(passBag, fx.shaderKey, bag);
                        fx.passUniforms.push_back(std::move(bag));
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
