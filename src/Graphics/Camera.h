#ifndef ISOMETRICGAME_CAMERA_H
#define ISOMETRICGAME_CAMERA_H

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <algorithm>

class Camera {
public:
    glm::vec3 target = {0.0f, 0.0f, 0.0f};
    glm::vec3 offset = {0.0f, 0.0f, 100.0f};
    glm::vec3 up = {0.0f, 0.0f, 1.0f};
    float zoom = 1.0f;
    float aspect = 16.0f / 9.0f;
    bool isometric = true;

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

    void Follow(const glm::vec3 &position, float speed = 0.1f) {
        target = glm::mix(target, position, speed);
    }

    void SnapTo(const glm::vec3 &position) {
        target = position;
    }

    glm::mat4 GetView() const {
        if (isometric) {
            // Hex grid is already laid out in XY so columns are vertical.
            // We rotate around X by -60 degrees to pitch the camera and achieve a 2:1 vertical squash,
            // while making Z map to "up" on the screen.
            glm::mat4 view = glm::mat4(1.0f);
            view = glm::rotate(view, glm::radians(-60.0f), glm::vec3(1, 0, 0));
            view = glm::translate(view, -target);
            return view;
        }

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
