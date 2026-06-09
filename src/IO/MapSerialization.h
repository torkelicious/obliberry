#ifndef OBLIBERRY_MAPSERIALIZATION_H
#define OBLIBERRY_MAPSERIALIZATION_H
#include <cstdint>
#include "../Map/Hex.h"

namespace MapIO {
    // #pragma pack stops the compiler stuffing invisible padding bytes :))
#pragma pack(push, 1)
    struct MapFileHeader {
        char magic[8] = {'O', 'B', 'L', 'I', 'H', 'E', 'X', 'M'}; // identifier
        uint16_t version = 1; // format may change so good to have
        uint32_t tileCount = 0;
    };

    struct SerializedTile {
        int16_t q;
        int16_t r;
        TileType type;
        bool walkable;
    };
#pragma pack(pop)

    // return true if sucessfull, false if not.
    bool Serialize(const std::string &path, const HexGrid &grid);

    bool Deserialize(const std::string &path, HexGrid &grid);

    bool CheckHeader(const MapFileHeader &header, const std::string &expected);

    // testing
    inline size_t CalculateExpectedFileSize(const size_t tileCount) {
        size_t headerSize = sizeof(MapFileHeader);
        size_t payloadSize = tileCount * sizeof(SerializedTile);
        return headerSize + payloadSize;
    }
}

#endif //OBLIBERRY_MAPSERIALIZATION_H
