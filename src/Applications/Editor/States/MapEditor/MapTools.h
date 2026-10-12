#pragma once
#include <cstdint>

namespace Editor {
    enum class MapTool : uint8_t { Paint, Erase, Select };
    enum class MapToolShape : uint8_t { RADIUS, RECT };
} // namespace Editor
