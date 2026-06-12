#include "MapSerialization.h"
#include <fstream>
#include <ios>
#include <iostream>

namespace MapIO {
    bool Serialize(const std::string &path, const HexGrid &grid) {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << path << "\n";
            return false;
        }
        MapFileHeader header;
        header.tileCount = static_cast<uint32_t>(grid.tiles.size());
        file.write(reinterpret_cast<const char *>(&header), sizeof(MapFileHeader));

        for (const auto &[coords,tile]: grid.tiles) {
            SerializedTile sTile{
                (coords.q),
                (coords.r),
                tile.type,
                tile.walkable
            };
            // reinterpret_cast to force compiler to treat struct as flat array of chars so fstream can write byte by byte
            file.write(reinterpret_cast<const char *>(&sTile), sizeof(SerializedTile));
        }
        file.close();
        std::cout << "Saved " << header.tileCount << " tiles to " << path << "\n";
        return true;
    }

    bool Deserialize(const std::string &path, HexGrid &grid) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << path << "\n";
            return false;
        }

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        MapFileHeader header;
        if (!file.read(reinterpret_cast<char *>(&header), sizeof(MapFileHeader))) {
            std::cerr << "Failed to read header from: " << path << "\n";
            return false;
        }

        if (!CheckHeader(header, MAP_FILE_MAGIC_STR)) {
            std::cerr << "Invalid map file format (header mismatch).\n";
            return false;
        }

        if (size_t expectedSize = CalculateExpectedFileSize(header.tileCount);
            expectedSize > static_cast<size_t>(fileSize)) {
            std::cerr << "Map file truncated or corrupt.\n";
            return false;
        }

        grid.Clear();
        for (uint32_t i = 0; i < header.tileCount; i++) {
            SerializedTile sTile{};
            if (!file.read(reinterpret_cast<char *>(&sTile), sizeof(SerializedTile))) {
                std::cerr << "Stream read error at tile " << i << "\n";
                break;
            }
            HexCoords coords{sTile.q, sTile.r};
            grid.EmplaceTile(coords, sTile.type, sTile.walkable);
        }
        file.close();
        std::cout << "Successfully loaded " << header.tileCount << " tiles.\n";
        return true;
    }

    bool CheckHeader(const MapFileHeader &header, const std::string &expected) {
        const std::string magicBytes(header.magic, 8);
        return magicBytes == expected;
    }
}
