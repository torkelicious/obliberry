#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 Position = {0.0f, 0.0f, 0.0f};
    float Zoom = 1.5f;

    float AngleX = 55.0f; // tilt down
    float AngleZ = 45.0f; // rotate world

    [[nodiscard]] glm::mat4 GetViewMatrix() const {
        auto view = glm::mat4(1.0f);
        // camera pan
        view = glm::translate(view, glm::vec3(-Position.x, -Position.y, 0.0f));
        return view;
    }

    [[nodiscard]] glm::mat4 GetProjectionMatrix(const float aspect) const {
        const float viewHeight = 20.0f / Zoom;
        const float viewWidth = viewHeight * aspect;

        return glm::ortho(
            -viewWidth * 0.5f, viewWidth * 0.5f,
            -viewHeight * 0.5f, viewHeight * 0.5f,
            -100.0f, 100.0f
        );
    }

    [[nodiscard]] glm::mat4 GetRotation() const {
        auto rot = glm::mat4(1.0f);
        // tilt down
        rot = glm::rotate(rot, glm::radians(AngleX), glm::vec3(1, 0, 0));
        // rotate around Z
        rot = glm::rotate(rot, glm::radians(AngleZ), glm::vec3(0, 0, 1));
        return rot;
    }

    [[nodiscard]] glm::mat4 GetVP(const float aspect) const {
        return GetProjectionMatrix(aspect) * GetRotation() * GetViewMatrix();
    }

    [[nodiscard]] glm::vec2 MouseToWorld(float mx, float my, float viewWidth, float viewHeight) const {
        const float aspect = viewWidth / viewHeight;

        // Correct for inverted Y in GLFW
        float adjustedMx = mx;
        float adjustedMy = my;

        // convert mouse pixels to NDC
        float x = 2.0f * adjustedMx / viewWidth - 1.0f;
        float y = 1.0f - 2.0f * adjustedMy / viewHeight;

        glm::vec4 nearClip(x, y, -1.0f, 1.0f);
        glm::vec4 farClip(x, y, 1.0f, 1.0f);

        // unproject
        glm::mat4 invVP = glm::inverse(GetVP(aspect));

        glm::vec4 nearWorld = invVP * nearClip;
        glm::vec4 farWorld = invVP * farClip;

        nearWorld /= nearWorld.w;
        farWorld /= farWorld.w;

        glm::vec3 rayDir = glm::vec3(farWorld) - glm::vec3(nearWorld);

        if (std::abs(rayDir.z) < 0.0001f) {
            return {nearWorld.x, nearWorld.y};
        }

        // ground plane = z0
        float t = -nearWorld.z / rayDir.z;
        glm::vec3 intersection = glm::vec3(nearWorld) + t * rayDir;

        return {intersection.x, intersection.y};
    }

    [[nodiscard]] glm::vec3 GetRightVector() const {
        glm::mat4 invRot = glm::inverse(GetRotation());
        return glm::vec3(invRot[0]); // local Right
    }

    [[nodiscard]] glm::vec3 GetUpVector() const {
        glm::mat4 invRot = glm::inverse(GetRotation());
        return glm::vec3(invRot[1]); // local Up
    }
};
