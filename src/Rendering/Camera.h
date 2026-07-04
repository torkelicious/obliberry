#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <cmath>

namespace Rendering {
    class Camera {
    public:
        glm::vec3 Position = {0.0f, 0.0f, 0.0f};
        float Zoom = 1.5f;

        [[nodiscard]] float GetAngleX() const { return m_AngleX; }
        [[nodiscard]] float GetAngleZ() const { return m_AngleZ; }

        void SetRotation(const float angleX, const float angleZ) {
            m_AngleX = angleX;
            m_AngleZ = angleZ;
            m_RotationDirty = true;
            m_InverseDirty = true;
        }

        [[nodiscard]] glm::mat4 GetViewMatrix() const {
            auto view = glm::mat4(1.0f);
            // camera pan
            view = glm::translate(view, glm::vec3(-Position.x, -Position.y, 0.0f));
            return view;
        }

        [[nodiscard]] glm::mat4 GetProjectionMatrix(const float aspect) const {
            const float viewHeight = 20.0f / Zoom;
            const float viewWidth = viewHeight * aspect;

            return glm::ortho(-viewWidth * 0.5f, viewWidth * 0.5f, -viewHeight * 0.5f, viewHeight * 0.5f, -100.0f,
                              100.0f);
        }

        [[nodiscard]] const glm::mat4 &GetRotation() const {
            if (m_RotationDirty) [[unlikely]] {
                auto rot = glm::mat4(1.0f);
                // tilt down
                rot = glm::rotate(rot, glm::radians(m_AngleX), glm::vec3(1, 0, 0));
                // rotate around z
                rot = glm::rotate(rot, glm::radians(m_AngleZ), glm::vec3(0, 0, 1));
                m_CachedRotation = rot;
                m_RotationDirty = false;
            }
            return m_CachedRotation;
        }

        [[nodiscard]] const glm::mat4 &GetInverseRotation() const {
            if (m_InverseDirty) [[unlikely]] {
                m_CachedInverseRotation = glm::inverse(GetRotation());
                m_InverseDirty = false;
            }
            return m_CachedInverseRotation;
        }

        [[nodiscard]] glm::mat4 GetVP(const float aspect) const {
            return GetProjectionMatrix(aspect) * GetRotation() * GetViewMatrix();
        }

        [[nodiscard]] glm::vec2 MouseToWorld(float mx, float my, float viewWidth, float viewHeight) const {
            const float aspect = viewWidth / viewHeight;

            // inverted Y in GLFW
            const float adjustedMx = mx;
            const float adjustedMy = my;

            // convert mouse pixels to NDC
            const float x = 2.0f * adjustedMx / viewWidth - 1.0f;
            const float y = 1.0f - 2.0f * adjustedMy / viewHeight;

            const glm::vec4 nearClip(x, y, -1.0f, 1.0f);
            const glm::vec4 farClip(x, y, 1.0f, 1.0f);

            // unproject
            const glm::mat4 invVP = glm::inverse(GetVP(aspect));

            glm::vec4 nearWorld = invVP * nearClip;
            glm::vec4 farWorld = invVP * farClip;

            nearWorld /= nearWorld.w;
            farWorld /= farWorld.w;

            const glm::vec3 rayDir = glm::vec3(farWorld) - glm::vec3(nearWorld);

            if (std::abs(rayDir.z) < 0.0001f) {
                return {nearWorld.x, nearWorld.y};
            }

            // ground plane = z0
            const float t = -nearWorld.z / rayDir.z;
            glm::vec3 intersection = glm::vec3(nearWorld) + t * rayDir;

            return {intersection.x, intersection.y};
        }

        [[nodiscard]] glm::vec3 GetRightVector() const {
            return glm::vec3(GetInverseRotation()[0]); // local Right
        }

        [[nodiscard]] glm::vec3 GetUpVector() const {
            return glm::vec3(GetInverseRotation()[1]); // local Up
        }

    protected:
        float m_AngleX = -55.0f; // tilt down
        float m_AngleZ = 45.0f; // rotate world

    private:
        mutable glm::mat4 m_CachedRotation{1.0f};
        mutable glm::mat4 m_CachedInverseRotation{1.0f};
        mutable bool m_RotationDirty = true;
        mutable bool m_InverseDirty = true;
    };
} // namespace Rendering
