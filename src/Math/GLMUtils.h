#pragma once
#include "glm/glm.hpp"
#include <algorithm>
#include <cmath>

namespace Math::GLMUtils {
    template <typename T> struct IsGlmVec : std::false_type {};

    template <glm::length_t L, typename T, glm::qualifier Q> struct IsGlmVec<glm::vec<L, T, Q>> : std::true_type {};

    template <typename T>
    concept GlmVec = IsGlmVec<std::remove_cvref_t<T>>::value;


    template <GlmVec T> bool VecIsFinite(const T &vec) {
        for (glm::length_t i = 0; i < vec.length(); ++i) {
            if (!std::isfinite(vec[i])) {
                return false;
            }
        }
        return true;
    }

    template <GlmVec T> void ClampVecPositive(T &vec) {
        for (glm::length_t i = 0; i < vec.length(); ++i) {
            vec[i] = std::max(vec[i], 0.0f);
        }
    }

} // namespace Math::GLMUtils
