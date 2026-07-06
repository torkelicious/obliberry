#include "MapSerialization.h"
#include <fstream>
#include <ios>
#include <iostream>

#include "VFS.h"

namespace IO::MapIO {
    bool Serialize(const std::string &path, const Map::HexGrid &grid) {
        std::filesystem::path resolvedPath = VFS::Resolve(path);
        std::ofstream file(resolvedPath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << path << "\n";
            return false;
        }
        MapFileHeader header;
        header.tileCount = static_cast<uint32_t>(grid.tiles.size());
        file.write(reinterpret_cast<const char *>(&header), sizeof(MapFileHeader));

        for (const auto &[coords, tile] : grid.tiles) {
            SerializedTile sTile{(coords.q), (coords.r), tile.type, tile.walkable};
            // reinterpret_cast to force compiler to treat struct as flat array of chars so fstream can write byte by
            // byte
            file.write(reinterpret_cast<const char *>(&sTile), sizeof(SerializedTile));
        }
        file.close();
        std::cout << "Saved " << header.tileCount << " tiles to " << path << "\n";
        return true;
    }

    bool Deserialize(const std::string &path, Map::HexGrid &grid) {
        const auto fileData = VFS::ReadVirtual(path);
        if (!fileData.has_value()) {
            std::cerr << "[MapIO] Failed to read map file from VFS: " << path << "\n";
            return false;
        }

        std::istringstream file(fileData.value(), std::ios::binary);
        const size_t fileSize = fileData.value().size();

        MapFileHeader header;
        if (!file.read(reinterpret_cast<char *>(&header), sizeof(MapFileHeader))) {
            std::cerr << "Failed to read header from: " << path << "\n";
            return false;
        }

        if (!CheckHeader(header, Core::MAP_FILE_MAGIC_STR)) {
            std::cerr << "Invalid map file format (header mismatch).\n";
            return false;
        }

        if (const size_t expectedSize = CalculateExpectedFileSize(header.tileCount); expectedSize > fileSize) {
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
            Map::HexCoords coords{sTile.q, sTile.r};
            grid.EmplaceTile(coords, sTile.type, sTile.walkable);
        }

        std::cout << "Successfully loaded " << header.tileCount << " tiles via VFS.\n";
        return true;
    }

    bool CheckHeader(const MapFileHeader &header, const std::string &expected) {
        return std::string_view(header.magic, 8) == expected;
    }
} // namespace IO::MapIO
