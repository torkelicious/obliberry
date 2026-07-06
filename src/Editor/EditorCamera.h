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

        void AdjustZoom(const float delta, const float minZoom = 0.1f, const float maxZoom = 50.0f) {
            Zoom += delta;
            Zoom = std::clamp(Zoom, minZoom, maxZoom);
        }

        void SaveState() {
            m_SavedPosition = Position;
            m_SavedZoom = Zoom;
            m_SavedAngleX = m_AngleX;
            m_SavedAngleZ = m_AngleZ;
        }

        void RestoreState() {
            Position = m_SavedPosition;
            Zoom = m_SavedZoom;
            SetRotation(m_SavedAngleX, m_SavedAngleZ);
        }

    private:
        glm::vec3 m_SavedPosition{0.0f};
        float m_SavedZoom = 1.5f;
        float m_SavedAngleX = 55.0f;
        float m_SavedAngleZ = 45.0f;
    };
} // namespace Editor
