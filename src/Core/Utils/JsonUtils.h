#pragma once
#include <nlohmann/json.hpp>

namespace Core::Utils::Json {

    inline void RoundJsonFloats(nlohmann::json &j, const int decimals = 3) {
        const float factor = std::pow(10.f, decimals);
        if (j.is_number_float()) {
            j = std::round(j.get<float>() * factor) / factor;
        } else if (j.is_array()) {
            for (auto &el : j)
                RoundJsonFloats(el, decimals);
        } else if (j.is_object()) {
            for (auto &[k, v] : j.items())
                RoundJsonFloats(v, decimals);
        }
    }
} // namespace Core::Utils::Json
