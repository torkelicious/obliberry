#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace Core::Utils::UUID {

    class UUIDGenerator {
    public:
        [[nodiscard]] static std::string Generate();

    private:
        using UUIDBytes = std::array<std::uint8_t, 16>;

        [[nodiscard]] static UUIDBytes RandomBytes16();
        [[nodiscard]] static std::string Format(const UUIDBytes &bytes);
    };

} // namespace Core::Utils::UUID
