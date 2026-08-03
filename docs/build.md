# Build Instructions

## Prerequisites

* **CMake:** Version 3.23 or newer.
* **Compiler:** with C++20 and C11 support (GCC, Clang, or MSVC).
* > *(Optional)* **Performance tools:** `ccache` and alternative linkers (`mold` or `lld`) are supported and recommended
  for faster builds on Linux.

## Clone the Repository

You must clone with submodules to fetch required libraries for the ObSL scripting language:

```bash
git clone --recurse-submodules https://github.com/torkelicious/obliberry
cd obliberry
```

> **Note:** A handful of additional dependencies (GLFW, GLM, nlohmann/json, nativefiledialog-extended, LZ4, FreeType)
> are *not* submodules, they are fetched automatically via CMake's `FetchContent` the first time you configure the
> project. This means an internet connection is required at configure time, and the initial `cmake --preset` step may
> take
> a while as these are downloaded into `.deps/`.

## Configure the Project

Generate the build files using one of the available CMake presets:

```bash
cmake --preset <preset> [options]
```

### Available Presets

**Linux**

* `linux-debug`: Debug build without optimizations.
* `linux-release`: Standard release build (includes LTO if supported).
* `linux-native`: Release build optimized specifically for your host CPU architecture.
* `linux-profile`: Release build with debug info (`RelWithDebInfo`) for profiling.

**macOS** (Requires macOS 11.0+)

* `macos-debug`
* `macos-release`

> **Note:** macOS is untested, but should work

**Windows**

* `windows-debug`: Debug build with Visual Studio 2022.
* `windows-release`: Release build with Visual Studio 2022.
* `windows-debug-2026`: Debug build with Visual Studio 2026.
* `windows-release-2026`: Release build with Visual Studio 2026.

### Build Options

You can customize the build by passing standard CMake options (`-D<OPTION>=<VALUE>`) during the configuration step:

* **`-DBUILD_PACK_TOOLS=ON|OFF`** (Default: `ON`)
  Toggles the compilation of `.obpak` packaging tools (`ob_packer`, `ob_unpacker`, `obsl_pack_run`).

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

**Packaging Tools** *(Requires `BUILD_PACK_TOOLS=ON`)*

* `ob_packer`: Builds `.obpak` packager tool.
* `ob_unpacker`: Extracts `.obpak` packages.
* `obsl_pack_run`: Run pre-parsed obsl scripts directly from .obpak archives.

## Running

After a successful build, the binaries end up in `<build>/bin`:

* `bin/obliberry_editor` - the editor. From its working directory it can open projects (the demo template is copied next
  to the editor binary under `bin/Templates`).
* `bin/internal/obliberry_runtime` - the runtime. See [the runtime usage notes](architecture.md) for command-line
  options (`-p`/`--project`, `-pk`/`--package`).

> **Note:** The runtime looks for a project (`project.json` + `graphics.json`) or a package (`data.obpak`) in its
> current working directory, or accepts explicit paths on the command line.
