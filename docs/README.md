# Obliberry Documentation

> VERY Work in progress. The engine is under development, so APIs, docs, file formats, and even the ObSL language
> itself may change

### Editor
If you want to get started with the Obliberry editor, please see: [Editor Docs](editor/index.md)


## Contents

| Section                                                     | Description                                                                         |
|-------------------------------------------------------------|-------------------------------------------------------------------------------------|
| [Build](build.md)                                           | Prerequisites, CMake presets, build options, and targets.                           |
| [Editor Guide](editor/usage.md)                             | Using the editor: modes, camera controls, panels, and keybinds.                     |
| [Architecture](architecture.md)                             | How the engine is organized: modules, the main loop, and the threading model.       |
| [Scripting : Getting Started](scripting/getting-started.md) | Write game logic in ObSL: the script lifecycle, hooks, and walkthroughs.            |
| [Scripting : API Reference](scripting/api-reference.md)     | Every EngineLib function exposed to scripts.                                        |
| [File Formats](formats/project-json.md)                     | `project.json`, `graphics.json`, scene files, `.obmap` maps, and `.obpak` packages. |
| [Third Party Licenses](THIRD_PARTY_LICENSES.md)             | License texts for every bundled and fetched dependency.                             |

## Where things live

| Path                               | What it is                                                                      |
|------------------------------------|---------------------------------------------------------------------------------|
| `src/Core`                         | Application, main loop, engine context, project, resource manager               |
| `src/Applications`                 | Editor, Runtime, and packaging tool executables                                 |
| `src/Scripting/EngineLib`          | The ObSL ↔ engine binding (see the [API reference](scripting/api-reference.md)) |
| `src/Scripting/EngineLib/Examples` | Example `.obsl` scripts                                                         |
| `Templates/DemoProject`            | A demo project template (assets, scenes, map)                                   |
| `Templates/Empty`                  | Minimal empty project template                                                  |
| `external/obsl`                    | The ObSL language submodule (interpreter, lexer, parser, stdlib)                |

## Quick links

- [ObSL language documentation](https://github.com/torkelicious/ObSL) the language itself is a separate MIT-licensed
  submodule. It ships with its own docs: `external/obsl/docs/ARCHITECTURE.md` and
  `external/obsl/docs/STANDARD_LIBRARY.md`.
- [Build instructions](build.md) start here to build the repo
