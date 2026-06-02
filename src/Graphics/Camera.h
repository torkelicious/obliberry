#ifndef ISOMETRICGAME_CAMERA_H
#define ISOMETRICGAME_CAMERA_H

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <algorithm>

class Camera {
public:
    glm::vec3 target = {0.0f, 0.0f, 0.0f};

    glm::vec3 offset = {0.0f, 500.0f, 500.0f};

    glm::vec3 up = {0.0f, 1.0f, 0.0f};

    float zoom = 1.0f;
    float aspect = 16.0f / 9.0f;

public:
    void SetAspect(float a) {
        aspect = (a > 0.0001f) ? a : 1.0f;
    }

    void SetZoom(float z) {
        zoom = std::clamp(z, 0.1f, 10.0f);
    }

    void AddZoom(float delta) {
        SetZoom(zoom + delta);
    }

    // Smooth follow
    void Follow(const glm::vec3 &position, float speed = 0.1f) {
        target = glm::mix(target, position, speed);
    }

    // Hard snap
    void SnapTo(const glm::vec3 &position) {
        target = position;
    }

    glm::mat4 GetView() const {
        glm::vec3 eye = target + offset;
        return glm::lookAt(eye, target, up);
    }

    glm::mat4 GetProjection() const {
        float base = 360.0f / zoom;

        return glm::ortho(
            -base * aspect, base * aspect,
            -base, base,
            -1000.0f, 1000.0f
        );
    }

    glm::mat4 GetVP() const {
        return GetProjection() * GetView();
    }
};

#endif