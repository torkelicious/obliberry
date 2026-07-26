#pragma once

#include <algorithm>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <memory>
#include <vector>
#include "Material.h"
#include <glm/ext/matrix_transform.hpp>

namespace Rendering {

    struct ParticleEmitterConfig {
        std::shared_ptr<Material> material = nullptr;

        glm::vec4 colorStart = {1.0f, 0.8f, 0.2f, 1.0f};
        glm::vec4 colorEnd = {1.0f, 0.2f, 0.0f, 0.0f};

        glm::vec3 velocityMin = {-1.0f, 2.0f, 0.0f};
        glm::vec3 velocityMax = {1.0f, 5.0f, 0.0f};
        glm::vec3 gravity = {0.0f, -9.8f, 0.0f};

        int maxParticles = 256;
        float emitRate = 50.0f;
        float lifetimeMin = 0.5f;
        float lifetimeMax = 1.5f;
        float sizeStartMin = 0.2f;
        float sizeStartMax = 0.2f;
        float sizeEndMin = 0.0f;
        float sizeEndMax = 0.0f;
        float rotationSpeedMin = 0.0f;
        float rotationSpeedMax = 0.0f;

        bool isBillboard = false;
    };
    class ParticlePool {
    public:
        explicit ParticlePool(ParticleEmitterConfig config) : m_Config(std::move(config)) { Reserve(m_Config.maxParticles); }

        ParticlePool() = default;

        // capacity / state
        void Reserve(const int capacity) {
            m_Position.resize(capacity);
            m_Velocity.resize(capacity);
            m_Gravity.resize(capacity);
            m_SizeStart.resize(capacity);
            m_SizeEnd.resize(capacity);
            m_ColorStart.resize(capacity);
            m_ColorEnd.resize(capacity);
            m_Life.resize(capacity);
            m_MaxLife.resize(capacity);
            m_RotationSpeed.resize(capacity);
            m_Rotation.resize(capacity);
        }

        void SetConfig(const ParticleEmitterConfig &config) {
            m_Config = config;
            if (static_cast<int>(m_Position.size()) < config.maxParticles) {
                Reserve(config.maxParticles);
            }
        }

        [[nodiscard]] const ParticleEmitterConfig &GetConfig() const { return m_Config; }

        [[nodiscard]] int AliveCount() const { return m_AliveCount; }

        [[nodiscard]] bool IsEmpty() const { return m_AliveCount == 0; }

        [[nodiscard]] bool IsFull() const { return m_AliveCount >= m_Config.maxParticles; }

        [[nodiscard]] int FreeCount() const { return m_Config.maxParticles - m_AliveCount; }

        // spawn

        int Spawn(const glm::vec3 position, const glm::vec3 velocity, const glm::vec3 grav, const float sizeS, const float sizeE, const glm::vec4 colorS, const glm::vec4 colorE, const float life,
                  const float rotSpeed = 0.0f) {
            if (m_AliveCount >= m_Config.maxParticles)
                return -1;

            const int i = m_AliveCount++;
            m_Position[i] = position;
            m_Velocity[i] = velocity;
            m_Gravity[i] = grav;
            m_SizeStart[i] = sizeS;
            m_SizeEnd[i] = sizeE;
            m_ColorStart[i] = colorS;
            m_ColorEnd[i] = colorE;
            m_Life[i] = life;
            m_MaxLife[i] = life;
            m_RotationSpeed[i] = rotSpeed;
            m_Rotation[i] = 0.0f;
            return i;
        }

        void Update(const float dt) {
            for (int i = 0; i < m_AliveCount; ++i) {
                m_Life[i] -= dt;
                if (m_Life[i] <= 0.0f) {
                    Kill(i);
                    --i;
                    continue;
                }

                m_Velocity[i] += m_Gravity[i] * dt;
                m_Position[i] += m_Velocity[i] * dt;
                m_Rotation[i] += m_RotationSpeed[i] * dt;
            }
        }


        [[nodiscard]] const glm::vec3 *GetPositionData() const { return m_Position.data(); }
        [[nodiscard]] const glm::vec3 *GetVelocityData() const { return m_Velocity.data(); }
        [[nodiscard]] const float *GetLifeData() const { return m_Life.data(); }
        [[nodiscard]] const float *GetMaxLifeData() const { return m_MaxLife.data(); }
        [[nodiscard]] const float *GetSizeStartData() const { return m_SizeStart.data(); }
        [[nodiscard]] const float *GetSizeEndData() const { return m_SizeEnd.data(); }
        [[nodiscard]] const glm::vec4 *GetColorStartData() const { return m_ColorStart.data(); }
        [[nodiscard]] const glm::vec4 *GetColorEndData() const { return m_ColorEnd.data(); }

        [[nodiscard]] float GetNormalizedAge(const int i) const { return 1.0f - m_Life[i] / m_MaxLife[i]; }

        // bulk transform build for instanced rendering
        void BuildTransforms(std::vector<glm::mat4> &out, const bool billboard = false, const glm::vec3 *camRight = nullptr, const glm::vec3 *camUp = nullptr) const {
            out.resize(m_AliveCount);
            for (int i = 0; i < m_AliveCount; ++i) {
                const float t = GetNormalizedAge(i);
                const float size = m_SizeStart[i] + (m_SizeEnd[i] - m_SizeStart[i]) * t;

                if (billboard && camRight && camUp) {
                    const float angle = m_Rotation[i];
                    const glm::vec3 fwd = glm::normalize(glm::cross(*camRight, *camUp));
                    const glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, fwd);
                    const glm::vec3 r = glm::vec3(rot * glm::vec4(*camRight, 0.0f)) * size;
                    const glm::vec3 u = glm::vec3(rot * glm::vec4(*camUp, 0.0f)) * size;
                    const glm::vec3 f = glm::cross(r, u);
                    const glm::vec3 center = m_Position[i] + u * 0.5f;
                    glm::mat4 model(1.0f);
                    model[0] = glm::vec4(r, 0.0f);
                    model[1] = glm::vec4(u, 0.0f);
                    model[2] = glm::vec4(f, 0.0f);
                    model[3] = glm::vec4(center, 1.0f);
                    out[i] = model;
                } else {
                    glm::mat4 model(1.0f);
                    model = glm::translate(model, m_Position[i]);
                    model = glm::rotate(model, m_Rotation[i], glm::vec3(0.0f, 0.0f, 1.0f));
                    model = glm::scale(model, glm::vec3(size));
                    out[i] = model;
                }
            }
        }

        void BuildTransformsAndColors(std::vector<glm::mat4> &outT, std::vector<glm::vec4> &outC, const bool billboard = false, const glm::vec3 *camRight = nullptr, const glm::vec3 *camUp = nullptr) const {
            outT.resize(m_AliveCount);
            outC.resize(m_AliveCount);
            for (int i = 0; i < m_AliveCount; ++i) {
                const float t = GetNormalizedAge(i);
                const float size = m_SizeStart[i] + (m_SizeEnd[i] - m_SizeStart[i]) * t;

                if (billboard && camRight && camUp) {
                    const float angle = m_Rotation[i];
                    const glm::vec3 fwd = glm::normalize(glm::cross(*camRight, *camUp));
                    const glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, fwd);
                    const glm::vec3 r = glm::vec3(rot * glm::vec4(*camRight, 0.0f)) * size;
                    const glm::vec3 u = glm::vec3(rot * glm::vec4(*camUp, 0.0f)) * size;
                    const glm::vec3 f = glm::cross(r, u);
                    const glm::vec3 center = m_Position[i] + u * 0.5f;
                    glm::mat4 model(1.0f);
                    model[0] = glm::vec4(r, 0.0f);
                    model[1] = glm::vec4(u, 0.0f);
                    model[2] = glm::vec4(f, 0.0f);
                    model[3] = glm::vec4(center, 1.0f);
                    outT[i] = model;
                } else {
                    glm::mat4 model(1.0f);
                    model = glm::translate(model, m_Position[i]);
                    model = glm::rotate(model, m_Rotation[i], glm::vec3(0.0f, 0.0f, 1.0f));
                    model = glm::scale(model, glm::vec3(size));
                    outT[i] = model;
                }
                outC[i] = m_ColorStart[i] + (m_ColorEnd[i] - m_ColorStart[i]) * t;
            }
        }

        void SpawnBurst(const glm::vec3 origin, const int count) {
            for (int n = 0; n < count; ++n) {
                if (m_AliveCount >= m_Config.maxParticles)
                    break;

                const glm::vec3 vel = glm::linearRand(m_Config.velocityMin, m_Config.velocityMax);
                const float life = glm::linearRand(m_Config.lifetimeMin, m_Config.lifetimeMax);
                const float sizeS = glm::linearRand(m_Config.sizeStartMin, m_Config.sizeStartMax);
                const float sizeE = glm::linearRand(m_Config.sizeEndMin, m_Config.sizeEndMax);
                const float rotSpeed = glm::linearRand(m_Config.rotationSpeedMin, m_Config.rotationSpeedMax);

                Spawn(origin, vel, m_Config.gravity, sizeS, sizeE, m_Config.colorStart, m_Config.colorEnd, life, rotSpeed);
            }
        }

        void Clear() { m_AliveCount = 0; }

    private:
        void Kill(const int index) {
            // swap and pop
            --m_AliveCount;
            if (index != m_AliveCount) {
                m_Position[index] = m_Position[m_AliveCount];
                m_Velocity[index] = m_Velocity[m_AliveCount];
                m_Gravity[index] = m_Gravity[m_AliveCount];
                m_SizeStart[index] = m_SizeStart[m_AliveCount];
                m_SizeEnd[index] = m_SizeEnd[m_AliveCount];
                m_ColorStart[index] = m_ColorStart[m_AliveCount];
                m_ColorEnd[index] = m_ColorEnd[m_AliveCount];
                m_Life[index] = m_Life[m_AliveCount];
                m_MaxLife[index] = m_MaxLife[m_AliveCount];
                m_RotationSpeed[index] = m_RotationSpeed[m_AliveCount];
                m_Rotation[index] = m_Rotation[m_AliveCount];
            }
        }

        ParticleEmitterConfig m_Config;

        int m_AliveCount = 0;
        std::vector<glm::vec3> m_Position;
        std::vector<glm::vec3> m_Velocity;
        std::vector<glm::vec3> m_Gravity;
        std::vector<float> m_SizeStart;
        std::vector<float> m_SizeEnd;
        std::vector<glm::vec4> m_ColorStart;
        std::vector<glm::vec4> m_ColorEnd;
        std::vector<float> m_Life;
        std::vector<float> m_MaxLife;
        std::vector<float> m_RotationSpeed;
        std::vector<float> m_Rotation;
    };

} // namespace Rendering
