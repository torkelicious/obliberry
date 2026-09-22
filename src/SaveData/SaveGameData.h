#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>

namespace Saves {

    inline constexpr std::uint8_t SAVE_FORMAT_VERSION = 1;

    using SaveValue = std::variant<bool, std::int64_t, double, std::string>;

    struct SaveData {
        // to keep uniuqe, shall be serialized with name of timestamp, also helps sorting?
        std::uint8_t version = SAVE_FORMAT_VERSION;
        std::string displayName;
        std::string currentScene;
        std::int64_t createdAtUtc = 0;
        std::int64_t updatedAtUtc = 0;
        std::unordered_map<std::string, SaveValue> values;
    };

} // namespace Saves
