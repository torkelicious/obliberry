#include "MapSerialization.h"
#include "Core/Utils/BitUtils.h"
#include "Core/Utils/Utils.h"
#include <fstream>
#include <ios>
#include <sstream>
#include "Logger/LoggerService.h"
#include "VFS/VFS.h"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "MapIO"

namespace IO::MapIO {

    using namespace Core::Utils::Bits;
    bool Serialize(const std::string &path, const Map::HexGrid &grid) {
        std::filesystem::path resolvedPath = VFS::Resolve(path);
        std::ofstream file(resolvedPath, std::ios::binary);
        if (!file.is_open()) {
            LOG_ERROR(LOG_WHO, "Failed to open file: " + path);
            return false;
        }
        MapFileHeader header;
        header.version = ToLittleEndian(header.version);
        header.tileCount = ToLittleEndian(static_cast<uint32_t>(grid.tiles.size()));
        file.write(reinterpret_cast<const char *>(&header), sizeof(MapFileHeader));

        for (const auto &[coords, tile] : grid.tiles) {
            SerializedTile sTile{.q = ToLittleEndian(coords.q), .r = ToLittleEndian(coords.r), .type = ToLittleEndian(tile.type), .walkable = tile.walkable};
            // reinterpret_cast to force compiler to treat struct as flat array of chars so fstream can write byte by
            // byte
            file.write(reinterpret_cast<const char *>(&sTile), sizeof(SerializedTile));
        }
        if (!file) {
            LOG_ERROR(LOG_WHO, "Write failure while saving map to " + path);
            return false;
        }
        file.close();
        LOG_INFO(LOG_WHO, "Saved " + std::to_string(grid.tiles.size()) + " tiles to " + path);
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

        const uint16_t nativeVersion = ToLittleEndian(header.version);
        if (nativeVersion != Core::MAP_FILE_VERSION) {
            LOG_ERROR(LOG_WHO, "Unsupported map version: " + std::to_string(nativeVersion));
            return false;
        }

        const uint32_t nativeTileCount = ToLittleEndian(header.tileCount);

        if (const size_t expectedSize = CalculateExpectedFileSize(nativeTileCount); expectedSize > fileSize) {
            LOG_ERROR(LOG_WHO, "Map file truncated or corrupt");
            return false;
        }

        grid.Clear();
        uint32_t readCount = 0;
        for (uint32_t i = 0; i < nativeTileCount; i++) {
            SerializedTile sTile{};
            if (!file.read(reinterpret_cast<char *>(&sTile), sizeof(SerializedTile))) {
                LOG_ERROR(LOG_WHO, "Stream read error at tile " + std::to_string(i) + " (expected " + std::to_string(nativeTileCount) + ")");
                grid.Clear();
                return false;
            }

            Map::HexCoords coords{ToLittleEndian(sTile.q), ToLittleEndian(sTile.r)};
            grid.EmplaceTile(coords, ToLittleEndian(sTile.type), sTile.walkable);
            ++readCount;
        }

        LOG_INFO(LOG_WHO, "Successfully loaded " + std::to_string(readCount) + " tiles via VFS");
        return true;
    }


    static bool CheckHeader(const MapFileHeader &header, const std::string_view &expected) { return std::string_view(header.magic, 8) == expected; }
} // namespace IO::MapIO
#pragma pop_macro("LOG_WHO")
