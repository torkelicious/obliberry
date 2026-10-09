# `.obpak` Package Format

`.obpak` is the engine's distribution container: it packs project data (scripts, scenes, maps, assets) into a single
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

Sections are packed consecutively with no alignment and no section headers. TOC entry `data_offset` values are
**relative to `blob_data_offset`** and cumulative (payloads are laid out in TOC order, back to back).

## File header `Package::FileHeader` (44 bytes)

| Field                 | Type       | Size | Meaning                                                                |
| --------------------- | ---------- | ---- | ---------------------------------------------------------------------- |
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
| ------------------- | ------------ | ---- | --------------------------------------------------------------------- |
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

- **LZ4 block format** (`LZ4_compress_default` / `LZ4_decompress_safe`).
- A payload is stored with `EntryFlags::Compressed` unless compression fails (returns ≤ 0). **There is no size-based
  fallback** incompressible data may end up compressed-but-larger on disk.
- Decompression must reproduce `uncompressed_size` exactly; otherwise the read fails.

### Per-type storage rules (from `AssetPacking`)

| File type                               | Stored as                        | Compressed?                  |
| --------------------------------------- | -------------------------------- | ---------------------------- |
| `.obsl` scripts                         | `SerializedAST` (pre-parsed AST) | yes (unless `--no-compress`) |
| `.json`                                 | `BinaryJSON` (msgpack)           | yes                          |
| `.png`, `.jpg`, `.jpeg`, `.mp3`, `.ogg` | `Media` (raw)                    | **never**                    |
| `.vert`, `.frag`, `.glsl`, `.shader`    | `ShaderSource` (raw)             | yes                          |
| `.obmap` / anything else                | `RawBinary`                      | yes                          |

This means `project.json`, the project-root `assets.json`, scene files, prefab files, and sprite-animation definitions
are all stored as `BinaryJSON` entries and decoded by `VFS::ReadVirtualJson()` at runtime.

Editor export also packs the engine's built-in shader helper files from `resources/shaders/` as `ShaderSource` entries
under the `engine/shaders/` prefix, so shader `#include`s of engine helpers keep working in exported games.

Script `using` imports are collected at pack time, rewritten to project-relative paths, and recorded in the dependency
graph.

## Reading

`ContainerReader` (`ContainerReader.cpp`):

- `open(path)` validates magic + version, reads the TOC and string table, then **memory-maps** the whole file (
  `MAP_PRIVATE` + `madvise(MADV_SEQUENTIAL)` on POSIX; `CreateFileMappingW` on Windows).
- `read(canonicalPath)` → `optional<string>` decompresses if flagged; bounds-checked.
- `read_view(canonicalPath)` → `optional<string_view>` zero-copy, **only for uncompressed entries**.
- `get_entry_paths()` / `print_entries()` listing helpers.
- Lookup is via a `string_view → index` map over the TOC.

## VFS integration

`IO::VFS::MountPackage(path)` opens the `.obpak` and sets the VFS into packaged mode; every `ReadVirtual`/
`ReadVirtualView` call is then served from the package (the disk is not consulted). This is how the runtime and tools
load a packaged game:

```
obliberry_runtime -pk game.obpak
```

## Tools

The packaging tools below, plus `ob_asset_migrator`, are built when `BUILD_PACK_TOOLS=ON` (default) and land in
`bin/tools/`.

### `ob_packer` create packages

```
ob_packer [options] <project_directory>
```

| Option                         | Meaning                                                                        |
| ------------------------------ | ------------------------------------------------------------------------------ |
| `-o, --output <file>`          | Output `.obpak` path (default: `<project_directory>.obpak`).                   |
| `-q, --quiet`                  | Suppress non-error output.                                                     |
| `--verbose`                    | Per-file detailed logging.                                                     |
| `--no-compress`                | Disable LZ4 for compressible types.                                            |
| `--strict`                     | Abort (exit 1) on dependency validation failures; otherwise they are warnings. |
| `-h, --help` / `-v, --version` | Help / version.                                                                |

Requires `<project_dir>/assets/scripts` to exist. Exit code `0` only if every file packed; `1` on failure or no files.

### `ob_unpacker` extract packages

```
ob_unpacker [options] <package.obpak> [output_directory]
```

| Option           | Meaning                                                  |
| ---------------- | -------------------------------------------------------- |
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

Editor export checks these rules against its required files. Ignoring a required file, or a directory containing one,
aborts export. Re-including a file makes it eligible for export; it does not add an otherwise unused file to the
dependency set.

## Dependency validation

At pack time, every `using "..."` in a script must resolve to another packed script. `DependencyGraph::validate` checks:

1. **Missing modules** `using` targets that aren't in the package.
2. **Cycles** circular `using` dependency chains.

Both are reported as errors; `ob_packer --strict` turns them into hard failures. Editor export always aborts on these
validation failures, missing required files, or unresolved catalog dependencies.

## Editor export

The editor's export flow (`ObpakTools.cpp`) builds a dependency set and writes it into `data.obpak` (with
`BINARY_NAME = "obliberry exporter"`). Unlike the directory-based `ob_packer`, it excludes files and catalog entries
that are not required by that set.

Collection starts with `project.json`, its `start_scene`, and every JSON scene under `assets/scenes/`. It includes:

- asset IDs used by the scenes' grid, post-processing, entity components, and UI;
- the scenes' map files and background music;
- attached scripts and their `using` imports;
- additional scenes, prefabs, and audio files referenced by supported script calls;
- material shaders/textures, animation JSON and sheet textures, and referenced shader `#include` files;
- the engine shader helpers under `engine/shaders/`.

Referenced scenes and prefabs are scanned for further dependencies. The package receives a generated `assets.json`
containing the retained definitions; the project's catalog and source files are not rewritten. All scenes under
`assets/scenes/` are included, even when they are not reachable from the start scene.

### Script references

Scripts are analysed from their parsed AST without executing them. String literals matching catalog IDs are retained,
including literals in branches or function bodies. Parentheses and concatenations of string literals can also be
resolved; variables and runtime expressions are not evaluated.

| Script use                                                                   | Export behavior                                   |
| ---------------------------------------------------------------------------- | ------------------------------------------------- |
| `image.SetTexture("player_sheet")` / `text.SetFont("dialogue_font")`         | Retains the referenced catalog asset.             |
| `image.SetTexture(textureId)` / `text.SetFont(fontId)`                       | Retains all textures / fonts in the catalog.      |
| `LoadScene("assets/scenes/next.json")`                                       | Includes and scans that scene.                    |
| `Instantiate("assets/prefabs/enemy.json")`                                   | Includes and scans that prefab.                   |
| `PlaySound2D("assets/audio/hit.ogg", 1.0)` / `PlayMusic(...)`                | Includes the referenced audio file.               |
| A file-loading call whose path cannot be resolved                            | Aborts export with the call name and script path. |
| An unclassified or indirect call, indexed lookup, or unsupported syntax node | Keeps the full asset catalog.                     |

For example, `LoadScene("assets/scenes/" + "next.json")` can be resolved, but `LoadScene(scenePath)` cannot, even
when `scenePath` was assigned a string literal earlier. Use direct literal paths in the file-loading calls. Keeping
the full catalog does not resolve unknown file paths, and export can still fail on a missing required file.

An unclassified-call message explains why the catalog is being kept; it is not itself an export failure. Keeping a
category or the full catalog includes those assets' files and dependencies, but does not eagerly load them at runtime.

### Export output

After packaging, the editor copies the runtime binary next to `data.obpak`, renamed to a sanitized version of the
project title (e.g. `My Game` → `My_Game`), plus a loose `graphics.json` when present. `obliberry_runtime` must be built
and located under the editor's `internal` directory for that copy to succeed. Export packages disk files without a
save prompt; save scene/map edits and asset drafts beforehand.
