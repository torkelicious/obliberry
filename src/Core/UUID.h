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

namespace Core {

    // UUID value type
    class UUID {
    public:
        using UUIDBytes = std::array<std::uint8_t, 16>;

        constexpr UUID() noexcept = default;

        static constexpr UUID FromBytes(const UUIDBytes &bytes) noexcept {
            UUID u;
            u.bytes_ = bytes;
            return u;
        }

        static std::optional<UUID> FromString(std::string_view txt) noexcept;

        [[nodiscard]] constexpr const UUIDBytes &bytes() const noexcept { return bytes_; }

        [[nodiscard]] constexpr bool IsNil() const noexcept { return *this == UUID(); }

        [[nodiscard]] std::string ToString() const;

        [[nodiscard]] constexpr bool operator==(const UUID &) const noexcept = default;
        [[nodiscard]] constexpr auto operator<=>(const UUID &) const noexcept = default;

    private:
        UUIDBytes bytes_{};
    };

    static_assert(sizeof(UUID) == 16);

    inline std::ostream &operator<<(std::ostream &os, const UUID &value) { return os << value.ToString(); }

    class UUIDGenerator {
    public:
        [[nodiscard]] static UUID Generate();

    private:
        [[nodiscard]] static UUID::UUIDBytes RandomBytes16();
    };

} // namespace Core

template <> struct std::hash<Core::UUID> {
    std::size_t operator()(const Core::UUID &uuid) const noexcept {
        std::uint64_t hash = 0xcbf29ce484222325ULL;
        for (const std::uint8_t byte : uuid.bytes()) {
            hash ^= byte;
            hash *= 0x100000001b3ULL;
        }
        return static_cast<std::size_t>(hash);
    }
};

template <> struct std::formatter<Core::UUID> : std::formatter<std::string_view> {
    auto format(const Core::UUID &value, std::format_context &ctx) const { return std::formatter<std::string_view>::format(value.ToString(), ctx); }
};

