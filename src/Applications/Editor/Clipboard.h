#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

namespace Editor {
    struct Clipboard {
        enum class Type : uint8_t { None, Entity, UIElement };

        Type type = Type::None;
        nlohmann::json payload;

        void Set(const Type t, nlohmann::json data) {
            type = t;
            payload = std::move(data);
        }

        void Clear() {
            type = Type::None;
            payload.clear();
        }

        [[nodiscard]] bool Has(const Type t) const { return type == t && !payload.empty(); }
    };
} // namespace Editor
