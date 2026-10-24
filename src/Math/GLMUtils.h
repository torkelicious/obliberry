#pragma once

#include <glm/glm.hpp>

#include <cmath>
#include <concepts>
#include <type_traits>

namespace Math::GLMUtils {

    template <typename T> struct IsGlmVec : std::false_type {};

    template <glm::length_t L, typename T, glm::qualifier Q> struct IsGlmVec<glm::vec<L, T, Q>> : std::true_type {};

    template <typename T>
    concept GlmVec = IsGlmVec<std::remove_cvref_t<T>>::value;


    template <typename T> struct IsGlmMat : std::false_type {};

    template <glm::length_t C, glm::length_t R, typename T, glm::qualifier Q> struct IsGlmMat<glm::mat<C, R, T, Q>> : std::true_type {};

    template <typename T>
    concept GlmMat = IsGlmMat<std::remove_cvref_t<T>>::value;


    template <typename T>
    concept GlmFloatVec = GlmVec<T> && std::floating_point<typename std::remove_cvref_t<T>::value_type>;

    template <typename T>
    concept GlmFloatMat = GlmMat<T> && std::floating_point<typename std::remove_cvref_t<T>::value_type>;


    template <GlmVec T> bool VecIsFinite(const T &vec) {
        for (glm::length_t i = 0; i < vec.length(); ++i) {
            if (!std::isfinite(vec[i])) {
                return false;
            }
        }
        return true;
    }

    template <GlmFloatMat T> bool MatIsFinite(const T &mat) {
        for (glm::length_t c = 0; c < mat.length(); ++c) {
            if (!VecIsFinite(mat[c])) {
                return false;
            }
        }
        return true;
    }

    template <GlmVec T> void ClampVecPositive(T &vec) {
        using Scalar = typename T::value_type;

        for (glm::length_t i = 0; i < vec.length(); ++i) {
            vec[i] = std::max(vec[i], Scalar{0});
        }
    }

    inline glm::vec3 MatrixScale(const glm::mat4 &matrix) { return {glm::length(glm::vec3(matrix[0])), glm::length(glm::vec3(matrix[1])), glm::length(glm::vec3(matrix[2]))}; }

    inline glm::vec3 MatrixPosition(const glm::mat4 &matrix) { return glm::vec3(matrix[3]); }

    inline bool MatrixHasShear(const glm::mat4 &matrix, const float tolerance = 1e-4f) {
        const glm::vec3 scale = MatrixScale(matrix);
        if (scale.x <= tolerance || scale.y <= tolerance || scale.z <= tolerance) {
            return false;
        }

        const glm::vec3 x = glm::vec3(matrix[0]) / scale.x;
        const glm::vec3 y = glm::vec3(matrix[1]) / scale.y;
        const glm::vec3 z = glm::vec3(matrix[2]) / scale.z;

        return std::abs(glm::dot(x, y)) > tolerance || std::abs(glm::dot(x, z)) > tolerance || std::abs(glm::dot(y, z)) > tolerance;
    }

    inline bool IsNonUniformScale(const glm::vec3 &scale, const float tolerance = 1e-4f) {
        return std::abs(scale.x - scale.y) > tolerance || std::abs(scale.x - scale.z) > tolerance || std::abs(scale.y - scale.z) > tolerance;
    }

} // namespace Math::GLMUtils
