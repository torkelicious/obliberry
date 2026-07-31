#pragma once

#include <cstdint>
#include "Map/Hex.h"

namespace IO::MapIO {
    // #pragma pack stops the compiler stuffing invisible padding bytes :))
#pragma pack(push, 1)
    struct MapFileHeader {
        char magic[8] = {'O', 'B', 'L', 'I', 'H', 'E', 'X', 'M'}; // identifier
        uint16_t version = 2; // format may change so good to have
        // even though the format itself didn't change, it is interpreted
        // differently so i still updated the version number (v2)!!
        uint32_t tileCount = 0;
    };

    struct SerializedTile {
        int16_t q;
        int16_t r;
        Map::TileType type;
        bool walkable;
    };
#pragma pack(pop)

    // return true if sucessfull, false if not.
    [[nodiscard]] bool Serialize(const std::string &path, const Map::HexGrid &grid);

    [[nodiscard]] bool Deserialize(const std::string &path, Map::HexGrid &grid);

    bool CheckHeader(const MapFileHeader &header, const std::string_view &expected);

    // testing
    inline size_t CalculateExpectedFileSize(const size_t tileCount) {
        constexpr size_t headerSize = sizeof(MapFileHeader);
        const size_t payloadSize = tileCount * sizeof(SerializedTile);
        return headerSize + payloadSize;
    }
} // namespace IO::MapIO
