#ifndef OBLIBERRY_CAMERA_H
#define OBLIBERRY_CAMERA_H

#include <limits>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "Core/Constants.h"

class Camera {
public:
    glm::vec3 Position = {0.0f, 0.0f, 0.0f};
    float Zoom = 1.5f;

    float AngleX = 55.0f; // tilt down
    float AngleZ = 45.0f; // rotate world

    glm::mat4 GetViewMatrix() const {
        glm::mat4 view = glm::mat4(1.0f);
        // camera pan
        view = glm::translate(view, glm::vec3(-Position.x, -Position.y, 0.0f));
        return view;
    }

    glm::mat4 GetProjectionMatrix(float aspect) const {
        float viewHeight = 20.0f / Zoom;
        float viewWidth = viewHeight * aspect;

        return glm::ortho(
            -viewWidth * 0.5f, viewWidth * 0.5f,
            -viewHeight * 0.5f, viewHeight * 0.5f,
            -100.0f, 100.0f
        );
    }

    glm::mat4 GetRotation() const {
        glm::mat4 rot = glm::mat4(1.0f);
        // tilt down
        rot = glm::rotate(rot, glm::radians(AngleX), glm::vec3(1, 0, 0));
        // rotate
        rot = glm::rotate(rot, glm::radians(AngleZ), glm::vec3(0, 0, 1));

        return rot;
    }

    glm::mat4 GetVP(float aspect = TARGET_ASPECT) const {
        if (Position != m_CachePos || Zoom != m_CacheZoom || aspect != m_CacheAspect) {
            m_CachedVP = GetProjectionMatrix(aspect) * GetRotation() * GetViewMatrix();
            m_CachePos = Position;
            m_CacheZoom = Zoom;
            m_CacheAspect = aspect;
        }
        return m_CachedVP;
    }

    glm::vec2 MouseToWorld(double mx, double my, float windowWidth, float windowHeight) const {
        // map strictly to the rendered map ignoring the black bars.
        float aspect = TARGET_ASPECT;
        float windowAspect = windowWidth / windowHeight;

        float viewWidth, viewHeight, viewX, viewY;
        if (windowAspect > aspect) {
            viewHeight = windowHeight;
            viewWidth = windowHeight * aspect;
            viewX = (windowWidth - viewWidth) / 2.0f;
            viewY = 0.0f;
        } else {
            viewWidth = windowWidth;
            viewHeight = windowWidth / aspect;
            viewX = 0.0f;
            viewY = (windowHeight - viewHeight) / 2.0f;
        }

        // mouse coordinates to be relative to the letterbox viewport
        float adjustedMx = mx - viewX;
        float adjustedMy = my - viewY;

        // convert mouse pixels to NDC
        float x = (2.0f * adjustedMx) / viewWidth - 1.0f;
        float y = 1.0f - (2.0f * adjustedMy) / viewHeight;

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

    glm::vec3 GetRightVector() const {
        glm::mat4 invRot = glm::inverse(GetRotation());
        return glm::vec3(invRot[0]); // local Right
    }

    glm::vec3 GetUpVector() const {
        glm::mat4 invRot = glm::inverse(GetRotation());
        return glm::vec3(invRot[1]); // local Up
    }

private:
    mutable glm::vec3 m_CachePos{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    };
    mutable float m_CacheZoom{-1.0f};
    mutable float m_CacheAspect{-1.0f};
    mutable glm::mat4 m_CachedVP{1.0f};
};

#endif
