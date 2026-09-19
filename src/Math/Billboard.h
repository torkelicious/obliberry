#pragma once
#include <glm/glm.hpp>

namespace Math {
    inline glm::mat4 MakeBillboardMatrix(const glm::vec3 &position, const float width, const float height, const glm::vec3 &right, const glm::vec3 &up) {
        const glm::vec3 forward = glm::cross(right, up);
        const glm::vec3 center = position + up * (height * 0.5f);
        glm::mat4 matrix(1.0f);
        matrix[0] = glm::vec4(right * width, 0.0f);
        matrix[1] = glm::vec4(up * height, 0.0f);
        matrix[2] = glm::vec4(forward, 0.0f);
        matrix[3] = glm::vec4(center, 1.0f);
        return matrix;
    }
} // namespace Math
