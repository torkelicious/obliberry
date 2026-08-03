# `.obmap` - Hex Map Files

`.obmap` files store hex grids: a header followed by a flat array of tile records. The map is **sparse** , only occupied
tiles are stored, there are no grid dimensions.

Implementation: `IO::MapIO` (`src/IO/MapSerialization.h/.cpp`). All structs are packed (`#pragma pack(1)`, zero padding)
and use **native endianness** with no checksum/CRC.

## Layout

```
Offset  Size             Content
0       8                magic "OBLIHEXM"
8       2                version (uint16) written as 2
10      4                tileCount (uint32)
14      6 × tileCount    tile records
```

Expected file size = `14 + 6 × tileCount` (`MapIO::CalculateExpectedFileSize`), used for truncation/corruption
detection.

## Header `MapFileHeader` (14 bytes)

| Field       | Type       | Size | Notes                                                 |
|-------------|------------|------|-------------------------------------------------------|
| `magic[8]`  | `char[8]`  | 8 B  | ASCII `"OBLIHEXM"` (also `Core::MAP_FILE_MAGIC_STR`). |
| `version`   | `uint16_t` | 2 B  | Written as `2`.                                       |
| `tileCount` | `uint32_t` | 4 B  | Number of `SerializedTile` records that follow.       |

## Tile record `SerializedTile` (6 bytes)

| Field      | Type      | Size | Notes                                                                   |
|------------|-----------|------|-------------------------------------------------------------------------|
| `q`        | `int16_t` | 2 B  | Hex column (odd-r offset).                                              |
| `r`        | `int16_t` | 2 B  | Hex row.                                                                |
| `type`     | `uint8_t` | 1 B  | Tile type id (`Map::TileType`); matches `grid.types` in the scene file. |
| `walkable` | `bool`    | 1 B  | Whether movement is allowed on this tile.                               |

## Semantics

* Coordinates use **odd-r offset, pointy-top hexes** (`Map::HexCoords`, see `src/Math/HexMath.h` for conversions).
* The grid in memory is a hash map (`HexGrid::tiles`), so **tile order on disk is non-deterministic** (hash order, not
  sorted).
* **Version is not validated on read** only the magic string is checked. A file with a bad magic is rejected (
  `"Invalid map file format (header mismatch)"`); any version number is accepted.
* No dimensions, chunk headers, or sections just header + flat tile array.
* Tiles are reconstructed with `HexGrid::EmplaceTile(coords, type, walkable)` on load; a walkability cache is maintained
  for pathfinding (A* in `HexGrid::FindPath`).

## Reading and writing

* `MapIO::Deserialize` reads through `VFS::ReadVirtual(path)`, so maps work identically from disk and from inside a
  mounted `.obpak`.
* `MapIO::Serialize` writes to `VFS::Resolve(path)` with `ofstream`.
* Maps conventionally live under `assets/maps/` (`Core::MAP_PATH`), extension `.obmap` (`Core::MAP_FILE_EXTENSION`).
* Scene files reference maps via the `grid` section's `map_file` key see [the scene format](scene-json.md#grid).
