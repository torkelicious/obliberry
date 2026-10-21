#pragma once

#include "Core/UUID.h"
#include <cstdint>
#include <functional>
namespace Scripting::Identity {

    //   entity / component wrapper: (kind << 32) | entity id (kind 0 = the entity)
    //   ui hashes uuid

    inline constexpr std::uint64_t kUITag = 1ULL << 63;

    constexpr std::uint64_t ForECS(const std::uint32_t entityId, const std::uint8_t kind) { return static_cast<std::uint64_t>(kind) << 32 | entityId; }
    inline std::uint64_t ForUI(const Core::UUID &uuid) { return kUITag | (std::hash<Core::UUID>{}(uuid) & 0x3FFFFFFFFFFFFFFFULL); }


} // namespace Scripting::Identity
