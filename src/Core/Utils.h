#ifndef OBLIBERRY_UTILS_H
#define OBLIBERRY_UTILS_H
#include <string>
#include <glm/glm.hpp>
#include "Renderer/Camera.h"

inline glm::mat4 GetBillboardMatrix(const glm::vec3 &position, float width, float height, const Camera &camera) {
    glm::vec3 right = camera.GetRightVector();
    glm::vec3 up = camera.GetUpVector();
    glm::vec3 forward = glm::cross(right, up);
    glm::vec3 renderCenter = position + up * (height * 0.5f);
    auto model = glm::mat4(1.0f);
    model[0] = glm::vec4(right * width, 0.0f);
    model[1] = glm::vec4(up * height, 0.0f);
    model[2] = glm::vec4(forward, 0.0f);
    model[3] = glm::vec4(renderCenter, 1.0f);
    return model;
}

namespace PathUtils {
    // accepts any number of string_views and joins them
    inline std::string Join(std::string_view p1, std::string_view p2, std::string_view p3 = "") {
        std::string result;
        result.reserve(p1.size() + p2.size() + p3.size());
        result += p1;
        result += p2;
        result += p3;
        return result;
    }
}

#endif //OBLIBERRY_UTILS_H
