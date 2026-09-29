#pragma once
#include "glm/geometric.hpp"
#include "glm/glm.hpp"
#include <algorithm>
#include <cmath>

namespace Math::GLMUtils {
    template <typename T> bool VecIsFinite(const T &vec) {
        for (glm::length_t i = 0; i < vec.length(); ++i) {
            if (!std::isfinite(vec[i])) {
                return false;
            }
        }
        return true;
    }

    template <typename T> void ClampVecPositive(T &vec) {
        for (glm::length_t i = 0; i < vec.length(); ++i) {
            vec[i] = std::max(vec[i], 0.0f);
        }
    }

} // namespace Math::GLMUtils
