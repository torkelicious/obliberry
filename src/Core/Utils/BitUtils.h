#pragma once

#include <bit>
#include <algorithm>
#include <array>
// bytes, bits and bobs...
namespace Core::Utils::Bits {
    template <typename T> constexpr T ToLittleEndian(T value) {
        if constexpr (std::endian::native == std::endian::big) {
            auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
            std::ranges::reverse(bytes);
            return std::bit_cast<T>(bytes);
        }
        return value; // no op
    }
} // namespace Core::Utils::Bits
