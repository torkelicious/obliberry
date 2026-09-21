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

    std::string UUIDGenerator::Generate() {
        UUIDBytes bytes = RandomBytes16();

        // UUID v4
        bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0fU) | 0x40U);

        // RFC 4122
        bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3fU) | 0x80U);

        return Format(bytes);
    }

    std::string UUIDGenerator::Format(const UUIDBytes &bytes) {
        constexpr char hex[] = "0123456789abcdef";

        std::string uuid;
        uuid.reserve(36);

        for (std::size_t i = 0; i < bytes.size(); ++i) {
            // 4-2-2-2-6, producing 8-4-4-4-12
            if (i == 4 || i == 6 || i == 8 || i == 10) {
                uuid.push_back('-');
            }
            const std::uint8_t byte = bytes[i];

            uuid.push_back(hex[(byte >> 4U) & 0x0fU]);
            uuid.push_back(hex[byte & 0x0fU]);
        }

        return uuid;
    }

    UUIDGenerator::UUIDBytes UUIDGenerator::RandomBytes16() {
        UUIDBytes bytes{};
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
