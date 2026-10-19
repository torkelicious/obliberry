#include "UUID.h"

#include <cerrno>
#include <cstddef>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#elif defined(__APPLE__)
#include <stdlib.h>
#else
#include <sys/random.h>
#endif

namespace Core::Utils::UUID {

    namespace {

        constexpr std::uint8_t InvalidHex = 0xFF;

        constexpr std::uint8_t HexValue(const char c) noexcept {
            if (c >= '0' && c <= '9') {
                return static_cast<std::uint8_t>(c - '0');
            }
            if (c >= 'a' && c <= 'f') {
                return static_cast<std::uint8_t>(c - 'a' + 0x0A);
            }
            if (c >= 'A' && c <= 'F') {
                return static_cast<std::uint8_t>(c - 'A' + 0x0A);
            }
            return InvalidHex;
        }

    } // namespace

    std::optional<uuid> uuid::FromString(std::string_view txt) noexcept {
        if (txt.size() != 36 || txt[8] != '-' || txt[13] != '-' || txt[18] != '-' || txt[23] != '-') {
            return std::nullopt;
        }

        UUIDBytes bytes{};
        std::size_t byte_idx = 0;
        bool high = true;

        for (std::size_t i = 0; i < txt.size(); ++i) {
            if (i == 8 || i == 13 || i == 18 || i == 23) {
                continue;
            }

            const std::uint8_t value = HexValue(txt[i]);
            if (value == InvalidHex) {
                return std::nullopt;
            }

            if (high) {
                bytes[byte_idx] = static_cast<std::uint8_t>(value << 4U);
            } else {
                bytes[byte_idx] |= value;
                ++byte_idx;
            }
            high = !high;
        }
        return FromBytes(bytes);
    }

    std::string uuid::ToString() const {
        constexpr char hex[] = "0123456789abcdef";
        std::string txt;
        txt.reserve(36);

        for (std::size_t i = 0; i < bytes_.size(); ++i) {
            if (i == 4 || i == 6 || i == 8 || i == 10) {
                txt.push_back('-');
            }
            const std::uint8_t byte = bytes_[i];
            txt.push_back(hex[(byte >> 4U) & 0x0FU]);
            txt.push_back(hex[byte & 0x0FU]);
        }
        return txt;
    }

    uuid UUIDGenerator::Generate() {
        uuid::UUIDBytes bytes = RandomBytes16();
        bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0FU) | 0x40U);
        bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3FU) | 0x80U);
        return uuid::FromBytes(bytes);
    }

    uuid::UUIDBytes UUIDGenerator::RandomBytes16() {
        uuid::UUIDBytes bytes{};
#ifdef _WIN32
        const NTSTATUS status = BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        if (!BCRYPT_SUCCESS(status)) {
            throw std::runtime_error("BCryptGenRandom failed while generating UUID");
        }

#elif defined(__APPLE__)
        arc4random_buf(bytes.data(), bytes.size());

#else
        std::size_t generated = 0;
        while (generated < bytes.size()) {
            const ssize_t result = getrandom(bytes.data() + generated, bytes.size() - generated, 0);
            if (result < 0) {
                if (errno == EINTR) {
                    continue;
                }
                throw std::system_error(errno, std::generic_category(), "getrandom failed while generating UUID");
            }

            if (result == 0) {
                throw std::runtime_error("getrandom returned zero bytes while generating UUID");
            }
            generated += static_cast<std::size_t>(result);
        }
#endif
        return bytes;
    }

} // namespace Core::Utils::UUID
