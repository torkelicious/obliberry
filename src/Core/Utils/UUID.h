#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

namespace Core::Utils::UUID {

    // uuid value type, confusing namespacing but whatever.
    class uuid {
    public:
        using UUIDBytes = std::array<std::uint8_t, 16>;

        constexpr uuid() noexcept = default;

        static constexpr uuid FromBytes(const UUIDBytes &bytes) noexcept {
            uuid u;
            u.bytes_ = bytes;
            return u;
        }

        static std::optional<uuid> FromString(std::string_view txt) noexcept;

        [[nodiscard]] constexpr const UUIDBytes &bytes() const noexcept { return bytes_; }

        [[nodiscard]] constexpr bool IsNil() const noexcept { return *this == uuid(); }

        [[nodiscard]] std::string ToString() const;

        [[nodiscard]] constexpr bool operator==(const uuid &) const noexcept = default;
        [[nodiscard]] constexpr auto operator<=>(const uuid &) const noexcept = default;

    private:
        UUIDBytes bytes_{};
    };

    static_assert(sizeof(uuid) == 16);

    inline std::ostream &operator<<(std::ostream &os, const uuid &value) { return os << value.ToString(); }

    class UUIDGenerator {
    public:
        [[nodiscard]] static uuid Generate();

    private:
        [[nodiscard]] static uuid::UUIDBytes RandomBytes16();
    };

} // namespace Core::Utils::UUID

template <> struct std::hash<Core::Utils::UUID::uuid> {
    std::size_t operator()(const Core::Utils::UUID::uuid &uuid) const noexcept {
        std::uint64_t hash = 0xcbf29ce484222325ULL;
        for (const std::uint8_t byte : uuid.bytes()) {
            hash ^= byte;
            hash *= 0x100000001b3ULL;
        }
        return static_cast<std::size_t>(hash);
    }
};

template <> struct std::formatter<Core::Utils::UUID::uuid> : std::formatter<std::string_view> {
    auto format(const Core::Utils::UUID::uuid &value, std::format_context &ctx) const { return std::formatter<std::string_view>::format(value.ToString(), ctx); }
};

// for testing
