#pragma once
#include <nlohmann/json.hpp>
#include <initializer_list>
#include <string_view>

namespace Core::Utils::Json {

    // Geometry must retain full precision.
    inline bool IsPrecisionCriticalKey(const std::string_view key) {
        for (const std::string_view critical : {"position", "rotation", "scale", "offset", "size", "matrix"}) {
            if (key == critical) {
                return true;
            }
        }
        return false;
    }

    inline void RoundJsonFloats(nlohmann::json &j, const int decimals = 3) {
        const float factor = std::pow(10.f, decimals);
        if (j.is_number_float()) {
            j = std::round(j.get<float>() * factor) / factor;
        } else if (j.is_array()) {
            for (auto &el : j)
                RoundJsonFloats(el, decimals);
        } else if (j.is_object()) {
            for (auto &[k, v] : j.items()) {
                if (IsPrecisionCriticalKey(k)) {
                    continue;
                }
                RoundJsonFloats(v, decimals);
            }
        }
    }
} // namespace Core::Utils::Json
