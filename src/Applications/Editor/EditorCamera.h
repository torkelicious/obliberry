#pragma once
#include "Rendering/Camera.h"
#include <algorithm>

namespace Editor {
    enum class CameraViewMode : uint8_t { Isometric, TopDown };

    class EditorCamera : public Rendering::Camera {
    public:
        EditorCamera() = default;

        CameraViewMode CurrentMode = CameraViewMode::Isometric;

        void ToggleViewMode() {
            if (CurrentMode == CameraViewMode::Isometric) {
                CurrentMode = CameraViewMode::TopDown;
                SetRotation(0.0f, 0.0f);
            } else {
                CurrentMode = CameraViewMode::Isometric;
                SetRotation(55.0f, 45.0f);
            }
        }

        void Pan(const float deltaX, const float deltaY, const float speedMultiplier = 1.0f) {
            const glm::vec3 right = GetRightVector();
            const glm::vec3 up = GetUpVector();
            const float speed = speedMultiplier / Zoom;
            Position += (right * deltaX + up * deltaY) * speed;
        }

        void KeyboardPan(const float deltaX, const float deltaY, const float speedMultiplier = 1.0f) {
            const glm::vec3 right = GetRightVector();
            const glm::vec3 up = GetUpVector();
            const float speed = speedMultiplier / Zoom;
            m_TargetVelocity = (right * deltaX + up * deltaY) * speed;
        }

        void StopKeyboardPan() { m_TargetVelocity = {0.0f, 0.0f, 0.0f}; }

        void UpdateSmooth(const float dt) {
            const float smoothFactor = 1.0f - std::pow(0.005f, dt * 10.0f);
            m_Velocity = glm::mix(m_Velocity, m_TargetVelocity, smoothFactor);
            Position += m_Velocity * dt;
            if (m_SmoothZoomActive) {
                Zoom = glm::mix(Zoom, m_SmoothZoomTarget, smoothFactor);
                if (std::abs(Zoom - m_SmoothZoomTarget) < 0.0001f) {
                    Zoom = m_SmoothZoomTarget;
                    m_SmoothZoomActive = false;
                }
            }
        }

        void AdjustZoom(const float delta, const float minZoom = 0.1f, const float maxZoom = 50.0f) {
            m_SmoothZoomTarget *= std::pow(1.1f, delta * 5.0f);
            m_SmoothZoomTarget = std::clamp(m_SmoothZoomTarget, minZoom, maxZoom);
            m_SmoothZoomActive = true;
        }

        void SaveState() {
            m_SavedPosition = Position;
            m_SavedZoom = Zoom;
            m_SavedAngleX = m_AngleX;
            m_SavedAngleZ = m_AngleZ;
            m_SmoothZoomTarget = Zoom;
            m_SmoothZoomActive = false;
        }

        void RestoreState() {
            Position = m_SavedPosition;
            Zoom = m_SavedZoom;
            m_SmoothZoomTarget = m_SavedZoom;
            m_SmoothZoomActive = false;
            SetRotation(m_SavedAngleX, m_SavedAngleZ);
        }

    private:
        glm::vec3 m_Velocity{0.0f};
        glm::vec3 m_TargetVelocity{0.0f};

        float m_SmoothZoomTarget = 1.5f;
        bool m_SmoothZoomActive = false;

        glm::vec3 m_SavedPosition{0.0f};
        float m_SavedZoom = 1.5f;
        float m_SavedAngleX = 55.0f;
        float m_SavedAngleZ = 45.0f;
    };
} // namespace Editor
