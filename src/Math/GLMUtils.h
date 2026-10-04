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

    template <GlmVec T> void ClampVecPositive(T &vec) {
        using Scalar = typename T::value_type;

        for (glm::length_t i = 0; i < vec.length(); ++i) {
            vec[i] = std::max(vec[i], Scalar{0});
        }
    }

} // namespace Math::GLMUtils
