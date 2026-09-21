#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
namespace Saves {

    constexpr uint8_t SAVE_FORMAT_VERSION = 1;

    using SaveValue = std::variant<bool, std::int64_t, double, std::string>;
    struct SaveData {
        // to keep uniuqe, shall be serialized with name of timestamp, also helps sorting?
        std::uint8_t version = 1;
        std::string displayName;
        std::string currentScene;
        std::int64_t createdAtUtc;
        std::int64_t updatedAtUtc;
        std::unordered_map<std::string, SaveValue> values;
    };

} // namespace Saves
