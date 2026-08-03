# `.obpak` Package Format

`.obpak` is the engine's distribution container: it packs a whole project (scripts, scenes, maps, assets) into a single
file that the runtime can mount directly. Layout: **file header → TOC → string table → blob**. Implemented in
`src/IO/Package/` (`Container.h`, `ContainerWriter.cpp`, `ContainerReader.cpp`).

All structs are packed (`#pragma pack(1)`) and use **native endianness** with no checksum/CRC.

## Layout

```
Offset 0              FileHeader (44 bytes)
Offset 44 (toc)       entry_count × TocEntry (40 bytes each)
...                   string table (string_table_size bytes raw concatenated paths, no terminators)
...                   blob (blob_data_offset) entry payloads back-to-back, no padding
```

Sections are packed consecutively with no alignment and no section headers. TOC entry `data_offset` values are *
*relative to `blob_data_offset`** and cumulative (payloads are laid out in TOC order, back to back).

## File header `Package::FileHeader` (44 bytes)

| Field                 | Type       | Size | Meaning                                                                |
|-----------------------|------------|------|------------------------------------------------------------------------|
| `magic[4]`            | `char[4]`  | 4 B  | ASCII `"OBPK"`.                                                        |
| `version`             | `uint16_t` | 2 B  | Must be `1`; the reader rejects anything else.                         |
| `flags`               | `uint16_t` | 2 B  | Written as `0`; not interpreted by the reader.                         |
| `entry_count`         | `uint32_t` | 4 B  | Number of `TocEntry` records.                                          |
| `toc_offset`          | `uint64_t` | 8 B  | File offset of the TOC always `44`.                                    |
| `string_table_offset` | `uint64_t` | 8 B  | File offset of the string table (right after the TOC).                 |
| `string_table_size`   | `uint64_t` | 8 B  | Byte size of the string table.                                         |
| `blob_data_offset`    | `uint64_t` | 8 B  | File offset of the first entry payload (right after the string table). |

## TOC entry `Package::TocEntry` (40 bytes)

| Field               | Type         | Size | Meaning                                                               |
|---------------------|--------------|------|-----------------------------------------------------------------------|
| `name_offset`       | `uint32_t`   | 4 B  | Offset into the string table.                                         |
| `name_length`       | `uint32_t`   | 4 B  | Path length (names are **not** NUL-terminated; offset + length only). |
| `data_offset`       | `uint64_t`   | 8 B  | Blob-relative offset of the payload.                                  |
| `compressed_size`   | `uint64_t`   | 8 B  | Stored payload size (compressed or not).                              |
| `uncompressed_size` | `uint64_t`   | 8 B  | Original size before compression.                                     |
| `type`              | `uint8_t`    | 1 B  | `EntryType` (below).                                                  |
| `flags`             | `uint8_t`    | 1 B  | `EntryFlags` (below).                                                 |
| `_pad[6]`           | `uint8_t[6]` | 6 B  | Explicit padding.                                                     |

```cpp
enum class EntryType : uint8_t { ScriptSource = 0, SerializedAST = 1, BinaryJSON = 2, RawBinary = 3, Media = 4, ShaderSource = 5 };
enum class EntryFlags : uint8_t { None = 0, Compressed = 1 << 0 };  // bit 0
```

## Compression

* **LZ4 block format** (`LZ4_compress_default` / `LZ4_decompress_safe`).
* A payload is stored with `EntryFlags::Compressed` unless compression fails (returns ≤ 0). **There is no size-based
  fallback** incompressible data may end up compressed-but-larger on disk.
* Decompression must reproduce `uncompressed_size` exactly; otherwise the read fails.

### Per-type storage rules (from `AssetPacking`)

| File type                               | Stored as                        | Compressed?                  |
|-----------------------------------------|----------------------------------|------------------------------|
| `.obsl` scripts                         | `SerializedAST` (pre-parsed AST) | yes (unless `--no-compress`) |
| `.json`                                 | `BinaryJSON` (msgpack)           | yes                          |
| `.png`, `.jpg`, `.jpeg`, `.mp3`, `.ogg` | `Media` (raw)                    | **never**                    |
| `.vert`, `.frag`, `.glsl`               | `ShaderSource` (raw)             | yes                          |
| `.obmap` / anything else                | `RawBinary`                      | yes                          |

Script `using` imports are collected at pack time, rewritten to project-relative paths, and recorded in the dependency
graph.

## Reading

`ContainerReader` (`ContainerReader.cpp`):

* `open(path)` validates magic + version, reads the TOC and string table, then **memory-maps** the whole file (
  `MAP_PRIVATE` + `madvise(MADV_SEQUENTIAL)` on POSIX; `CreateFileMappingW` on Windows).
* `read(canonicalPath)` → `optional<string>` decompresses if flagged; bounds-checked.
* `read_view(canonicalPath)` → `optional<string_view>` zero-copy, **only for uncompressed entries**.
* `get_entry_paths()` / `print_entries()` listing helpers.
* Lookup is via a `string_view → index` map over the TOC.

## VFS integration

`IO::VFS::MountPackage(path)` opens the `.obpak` and sets the VFS into packaged mode; every `ReadVirtual`/
`ReadVirtualView` call is then served from the package (the disk is not consulted). This is how the runtime and tools
load a packaged game:

```
obliberry_runtime -pk game.obpak
```

## Tools

All three tools are built when `BUILD_PACK_TOOLS=ON` (default) and land in `bin/tools/`.

### `ob_packer` create packages

```
ob_packer [options] <project_directory>
```

| Option                         | Meaning                                                                        |
|--------------------------------|--------------------------------------------------------------------------------|
| `-o, --output <file>`          | Output `.obpak` path (default: `<project_directory>.obpak`).                   |
| `-q, --quiet`                  | Suppress non-error output.                                                     |
| `--verbose`                    | Per-file detailed logging.                                                     |
| `--no-compress`                | Disable LZ4 for compressible types.                                            |
| `--strict`                     | Abort (exit 1) on dependency validation failures; otherwise they are warnings. |
| `-h, --help` / `-v, --version` | Help / version.                                                                |

Requires `<project_dir>/assets/scripts` to exist. Exit code `0` only if every file packed; `1` on failure or no files.

### `ob_unpacker` extract packages

```
ob_unpack [options] <package.obpak> [output_directory]
```

| Option           | Meaning                                                  |
|------------------|----------------------------------------------------------|
| `-l, --list`     | List contents without extracting.                        |
| `-r, --readable` | Decode `.json` entries from msgpack back to pretty JSON. |
| `-q, --quiet`    | Suppress output.                                         |

Default output directory is the package file stem.

### `obsl_pack_run` run a script from a package

```
obsl_pack_run [options] <package.obpak> <entry_script_path>
```

Reads a `SerializedAST` entry, deserializes it, and runs it with a module loader that resolves `using` imports directly
from the archive, no filesystem needed.

## `.pakignore` rules

`ob_packer` honors a `.pakignore` file in the project root (built-ins are prepended first, so a rule can re-include
them: `imgui.ini`, `.DS_Store`, `graphics.json`, `.pakignore`). Syntax:

```
# comment
!pattern              negates (re-includes) a previously ignored path
pattern/              matches directories only
/pattern              anchored to the ignore file's directory
pattern with '/'      matched against the full path relative to the ignore file
pattern without '/'   matched against the basename at any depth
* ? **                glob wildcards ('**' crosses directory boundaries)
\x                    escapes a literal 'x'
```

Rules are evaluated in order **last matching rule wins**.

## Dependency validation

At pack time, every `using "..."` in a script must resolve to another packed script. `DependencyGraph::validate` checks:

1. **Missing modules**  `using` targets that aren't in the package.
2. **Cycles** circular `using` dependency chains.

Both are reported as errors; `--strict` turns them into hard failures.

## Editor export

The editor's export flow (`ObpakTools.cpp`) does the equivalent of `ob_packer` into `data.obpak` (with
`BINARY_NAME = "obliberry exporter"`), then copies the runtime binary next to it, renamed to a sanitized version of the
project title (e.g. `My Game` → `My_Game`), plus `graphics.json`. `obliberry_runtime` must be built for export to work.
