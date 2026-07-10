#include "MapSerialization.h"
#include <fstream>
#include <ios>
#include "Core/LoggerService.h"
#include "VFS.h"

constexpr auto LOG_WHO = "MapIO";

namespace IO::MapIO {
    bool Serialize(const std::string &path, const Map::HexGrid &grid) {
        std::filesystem::path resolvedPath = VFS::Resolve(path);
        std::ofstream file(resolvedPath, std::ios::binary);
        if (!file.is_open()) {
            LOG_ERROR(LOG_WHO, "Failed to open file: " + path);
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
        LOG_INFO(LOG_WHO, "Saved " + std::to_string(header.tileCount) + " tiles to " + path);
        return true;
    }

    bool Deserialize(const std::string &path, Map::HexGrid &grid) {
        const auto fileData = VFS::ReadVirtual(path);
        if (!fileData.has_value()) {
            LOG_ERROR(LOG_WHO, "Failed to read map file from VFS: " + path);
            return false;
        }

        std::istringstream file(fileData.value(), std::ios::binary);
        const size_t fileSize = fileData.value().size();

        MapFileHeader header;
        if (!file.read(reinterpret_cast<char *>(&header), sizeof(MapFileHeader))) {
            LOG_ERROR(LOG_WHO, "Failed to read header from: " + path);
            return false;
        }

        if (!CheckHeader(header, Core::MAP_FILE_MAGIC_STR)) {
            LOG_ERROR(LOG_WHO, "Invalid map file format (header mismatch)");
            return false;
        }

        if (const size_t expectedSize = CalculateExpectedFileSize(header.tileCount); expectedSize > fileSize) {
            LOG_ERROR(LOG_WHO, "Map file truncated or corrupt");
            return false;
        }

        grid.Clear();
        for (uint32_t i = 0; i < header.tileCount; i++) {
            SerializedTile sTile{};
            if (!file.read(reinterpret_cast<char *>(&sTile), sizeof(SerializedTile))) {
                LOG_ERROR(LOG_WHO, "Stream read error at tile " + std::to_string(i));
                break;
            }
            Map::HexCoords coords{sTile.q, sTile.r};
            grid.EmplaceTile(coords, sTile.type, sTile.walkable);
        }

        LOG_INFO(LOG_WHO, "Successfully loaded " + std::to_string(header.tileCount) + " tiles via VFS");
        return true;
    }

    bool CheckHeader(const MapFileHeader &header, const std::string &expected) {
        return std::string_view(header.magic, 8) == expected;
    }
} // namespace IO::MapIO
