# Build Instructions

## Prerequisites

* **CMake:** Version 3.25 or newer for the supplied presets (the project itself declares a 3.23 minimum).
* **Compiler:** with C++20 and C11 support (GCC, Clang, or MSVC).
* **Ninja:** Required by the Linux, macOS, and `windows-ninja-*` presets. On Windows, use an MSVC developer shell.
* > *(Optional)* **Performance tools:** `ccache` and alternative linkers (`mold` or `lld`) are supported and recommended
  for faster builds on Linux.

## Clone the Repository

You must clone with submodules to fetch required libraries for the ObSL scripting language:

```bash
git clone --recurse-submodules https://github.com/torkelicious/obliberry
cd obliberry
```

> **Note:** Most dependencies are *not* submodules, they are fetched automatically via CMake's `FetchContent` the first
> time you configure the
> project. This means an internet connection is required at configure time, and the initial `cmake --preset` step may
> take a while as these are downloaded into `_deps/`.

## Configure the Project

Generate the build files using one of the available CMake presets:

```bash
cmake --preset <preset> [options]
```

### Available Presets

**Linux**

* `linux-debug`: Debug build without optimizations.
* `linux-release`: Standard release build (includes LTO if supported).
* `linux-profile`: Release build with debug info (`RelWithDebInfo`) for profiling.
* `linux-sanitizers`: Debug build with AddressSanitizer and UndefinedBehaviorSanitizer.

**macOS** (Requires macOS 11.0+)

* `macos-debug`
* `macos-release`
* `macos-sanitizers`

> **Note:** macOS is included in the build and test workflows. Rendering integration tests run on Linux.

**Windows**

* `windows-debug`: Debug build with Visual Studio 2022.
* `windows-release`: Release build with Visual Studio 2022.
* `windows-debug-2026`: Debug build with Visual Studio 2026.
* `windows-release-2026`: Release build with Visual Studio 2026.
* `windows-ninja-debug`: Debug build with Ninja and the MSVC `cl` compiler (used by CI).
* `windows-ninja-release`: Release build with Ninja and the MSVC `cl` compiler (used by CI).

For Visual Studio, configure with `windows-default` (2022) or `windows-default-2026` (2026), then build with the
corresponding debug/release preset above. The Ninja presets use the same name for configuring and building.

### Build Options

You can customize the build by passing standard CMake options (`-D<OPTION>=<VALUE>`) during the configuration step:

* **`-DBUILD_PACK_TOOLS=ON|OFF`** (Default: `ON`)
  Toggles the compilation of project/package tools (`ob_packer`, `ob_unpacker`, `obsl_pack_run`,
  `ob_asset_migrator`).

* **`-DBUILD_TESTING=ON|OFF`** (Default: `ON`)
  Toggles the `obliberry_tests` target and CTest discovery. Set `OFF` for an application-only build.

* **`-DOBLIBERRY_TEST_RENDERING=ON|OFF`** (Default: `OFF`, requires `BUILD_TESTING=ON`)
  Adds rendering integration tests that require an OpenGL 4.3 context.

* **`-DENGINE_ARCH_LEVEL="<arch>"`** (Default: `"x86-64-v2"`)
  Sets the target CPU architecture baseline for GCC/Clang on x86 architectures. Common options include `x86-64-v2`,
  `x86-64-v3`, or `native`.

* **`-DENABLE_UNITY_BUILD=ON|OFF`** (Default: `OFF`)
  Enables Unity (jumbo) builds for `obliberry_engine` and `obliberry_editor` to speed up compilation times. *Note: might
  be broken on Windows.*

* **`-DCMAKE_OSX_DEPLOYMENT_TARGET="<version>"`** (Default: `"11.0"`, macOS only)
  Sets the minimum supported macOS deployment version.

## Build the Project

It is **highly recommended to build all targets**. To do so, omit the target flag:

```bash
cmake --build --preset <preset>
```

To build a specific target, pass the `--target` flag:

```bash
cmake --build --preset <preset> --target <target_name>
```

### Available Targets

**Applications**

* `obliberry_editor`: The Editor application.
  > *(Note: `obliberry_runtime` must also be built for project exporting to function.)*
* `obliberry_runtime`: The standalone runtime application.

**Project and Packaging Tools** *(Requires `BUILD_PACK_TOOLS=ON`)*

* `ob_packer`: Builds `.obpak` packager tool.
* `ob_unpacker`: Extracts `.obpak` packages.
* `obsl_pack_run`: Run pre-parsed obsl scripts directly from .obpak archives.
* `ob_asset_migrator`: Moves legacy per-scene asset definitions into the project-root `assets.json` catalog.

The tools are written to `<build>/bin/tools/`.

**Tests** *(Requires `BUILD_TESTING=ON`)*

* `obliberry_tests`: Google Test suite, with individual cases discovered by CTest.

### Migrating legacy asset definitions

Older projects stored complete asset definitions in each scene file. Validate and migrate a project with:

```bash
<build>/bin/tools/ob_asset_migrator "/path/to/project" --dry-run
<build>/bin/tools/ob_asset_migrator "/path/to/project" --strip-scenes
```

On Windows:

```powershell
<build>\bin\tools\ob_asset_migrator.exe "C:\path\to\project" --dry-run
<build>\bin\tools\ob_asset_migrator.exe "C:\path\to\project" --strip-scenes
```

`--dry-run` performs validation without writing. The default migration merges definitions into `assets.json` and
leaves legacy scene sections intact. `--strip-scenes` also removes those sections after creating backups. See the
[`assets.json` format](formats/assets-json.md#migrating-old-projects) for details.

## Tests

Configure and build the test target, then run CTest from the project root.

Linux:

```bash
cmake --preset linux-debug -DBUILD_TESTING=ON -DBUILD_PACK_TOOLS=OFF
cmake --build --preset linux-debug --target obliberry_tests --parallel 2
ctest --test-dir build/linux-debug --output-on-failure --no-tests=error
```

Windows, from an MSVC developer shell:

```powershell
cmake --preset windows-ninja-debug -DBUILD_TESTING=ON -DBUILD_PACK_TOOLS=OFF
cmake --build --preset windows-ninja-debug --target obliberry_tests --parallel 2
ctest --test-dir build/windows-ninja-debug --output-on-failure --no-tests=error
```

macOS:

```bash
cmake --preset macos-debug -DBUILD_TESTING=ON -DBUILD_PACK_TOOLS=OFF
cmake --build --preset macos-debug --target obliberry_tests --parallel 2
ctest --test-dir build/macos-debug --output-on-failure --no-tests=error
```

The suite covers ECS entities and hierarchy, scenes, maps, animation, ObSL and engine bindings, asset catalog/loading,
VFS and package reads, configuration, editor command undo/redo, and missing/malformed audio. Rendering tests are
optional locally: add `-DOBLIBERRY_TEST_RENDERING=ON` when configuring. They skip if a context cannot be created unless
`OBLIBERRY_REQUIRE_RENDERING_TESTS=1` is set. Audio tests may skip if no backend initializes.

On Linux, headless rendering tests need Xvfb, `xauth`, and Mesa OpenGL support. After configuring with rendering
enabled,
run:

```bash
LIBGL_ALWAYS_SOFTWARE=1 OBLIBERRY_REQUIRE_RENDERING_TESTS=1 xvfb-run -a ctest --test-dir build/linux-debug --output-on-failure --no-tests=error
```

### Continuous integration

`.github/workflows/tests.yml` runs on pull requests, pushes to `main`/`master`, and manual dispatch:

| Platform | Preset                | Rendering tests                                                                     |
|----------|-----------------------|-------------------------------------------------------------------------------------|
| Linux    | `linux-debug`         | Enabled under Xvfb with Mesa software rendering; unavailable contexts fail the job. |
| Windows  | `windows-ninja-debug` | Disabled.                                                                           |
| macOS    | `macos-debug`         | Disabled.                                                                           |

Both `tests.yml` and `build.yml` cache fetched dependencies under `.deps` using `FETCHCONTENT_BASE_DIR`, and cache
compiler results with `sccache`. Dependency keys include the CMake definitions; compiler-cache keys include the commit
and restore earlier entries for the same platform, architecture, and preset. Windows debug tests use embedded debug
information (`/Z7`) for sccache compatibility.

Test logs are uploaded even after a failed run when available, with seven-day retention. The build/release workflow
uses release presets with `BUILD_TESTING=OFF`; it runs on `v*` tags or manual dispatch. Manual runs publish a release
only when `publish_release` is enabled, and newly created release tags point to the built commit.

## Running

After a successful build, the binaries end up in `<build>/bin`:

* `bin/obliberry_editor` - the editor. From its working directory it can open projects (the demo template is copied next
  to the editor binary under `bin/Templates`).
* `bin/internal/obliberry_runtime` - the runtime. See [the runtime usage notes](architecture.md) for command-line
  options (`-p`/`--project`, `-pk`/`--package`).

> **Note:** The runtime looks for a project (`project.json` + `graphics.json`) or a package (`data.obpak`) in its
> current working directory, or accepts explicit paths on the command line.
