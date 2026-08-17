# The Obliberry Game Engine

A Fallout-inspired isometric game engine for hex-grid games, written in C++20 with OpenGL, featuring a visual editor,
an ECS core, [its own scripting language](https://github.com/torkelicious/ObSL), and one-file packaging.

> **note:** still in active development. Scripting APIs, file formats, and the ObSL language can still change.

![Editor application in use](docs/img/demo-projects.gif)

<sup><sub>Yes there is a lighting system bug in this gif, it has since been fixed :)</sub></sup>

## Quick start

Download the latest prebuilt binaries from
**the [releases page](https://github.com/torkelicious/obliberry/releases/latest).**
They come with the editor, runtime, tools, and project templates. Unzip and run `obliberry_editor` to get started.

### [How to use the editor](docs/editor/usage.md)

Want a look at something made with the engine? There is also a prebuilt Demo project at the
[demo release](https://github.com/torkelicious/obliberry/releases/tag/demo): download the version for your OS and run
the `DemoProject` executable.

To build the engine from source instead, see the [build instructions](docs/build.md).

> **note:** this is developed and tested on Linux, sometimes tested in a Windows VM. macOS is expected to maybe work
> but is untested.


## Features

- Visual editor with Hub, Edit, Play, and MapEditor states, gizmo transforms, and pixel-based entity picking.
- ECS core with versioned entity handles and dense component pools.
- Hex-grid maps (odd-r offset, pointy-top) with built-in A* pathfinding.
- ObSL scripting: a small embedded language with hot reload, parallel execution, and lifecycle hooks like
  `on_update(dt)`.
- Ship a game as a single `.obpak` package with LZ4 compression and pre-parsed scripts.
- Built-in scene UI (text, buttons, images) and audio (2D sound effects and looping music).
- Basic particle and lighting systems.

## How it works

Obliberry splits the main loop from rendering: the main thread updates game logic, ECS systems, and scripts, submits a
frame, and hands it to a dedicated render thread with its own GL context. Frames are double-buffered at the frame level,
so the main thread prepares the next frame while the previous one is still being drawn. ObSL scripts run in parallel
across a thread pool, one interpreter per worker, but they cannot mutate the ECS directly: writes are routed through
command buffers that the main thread flushes, which keeps parallel scripts safe without locking every component access.

Hex maps use an odd-r offset layout with pointy-top hexes, a compact binary format (`.obmap`), and A* pathfinding over
the grid. Projects are packaged into a single `.obpak` file: a header, a table of contents, a string
table, and an LZ4-compressed blob. Scripts are pre-parsed at pack time and stored as serialized ASTs, so a packaged game
skips parsing on startup, and media files are stored uncompressed by design since compressing textures and audio rarely
pays off.

## Credits and thanks

Open-source projects used:

| Project                                                                        | Where it is used                                                |
|--------------------------------------------------------------------------------|-----------------------------------------------------------------|
| [ObSL](https://github.com/torkelicious/ObSL)                                   | The embedded scripting language (submodule, MIT), made by me :D |
| [GLAD](https://github.com/Dav1dde/glad)                                        | OpenGL loader                                                   |
| [GLFW](https://github.com/glfw/glfw)                                           | Window and input                                                |
| [GLM](https://github.com/g-truc/glm)                                           | Math                                                            |
| [stb_image](https://github.com/nothings/stb/tree/master)                       | Image loading                                                   |
| [Dear ImGui](https://github.com/ocornut/imgui)                                 | Editor UI                                                       |
| [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)                        | Gizmo transforms                                                |
| [nlohmann/json](https://github.com/nlohmann/json)                              | JSON serialization                                              |
| [miniaudio](https://github.com/mackron/miniaudio)                              | Audio                                                           |
| [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended) | File dialogs                                                    |
| [FreeType](https://github.com/freetype/freetype)                               | Font rendering                                                  |
| [Open Sans](https://github.com/googlefonts/opensans)                           | Demo project UI font (SIL OFL 1.1)                              |
| [LZ4](https://github.com/lz4/lz4)                                              | `.obpak` compression                                            |

Full license texts for all of the above: [THIRD_PARTY_LICENSES.md](docs/THIRD_PARTY_LICENSES.md).

Learning resources that shaped the codebase:

* [LearnOpenGL](http://learnopengl.com/) and [docs.gl](https://docs.gl/) for OpenGL.
* [Red Blob Games](https://www.redblobgames.com/grids/hexagons/) for hex grid math.
* [Crafting Interpreters](https://craftinginterpreters.com/contents.html) for the ObSL interpreter.
* [A Quick Guide to Interpreter Design in Modern C++](https://simplifycpp.org/books/cpp/Quick_Guide_to_Interpreter_Design_by_Modern_CPP.pdf)
  by Ayman Alheraki, for the ObSL interpreter.
* For the ECS: [C++ Game Engine Design: Basics to Advanced](https://codezup.com/cpp-game-engine-design-basics-advanced/),
  [A Simple Entity Component System (ECS) [C++]](https://austinmorlan.com/posts/entity_component_system/),
  [An Entity Component System from Scratch](https://www.codingwiththomas.com/blog/an-entity-component-system-from-scratch), and
  [Making a Simple ECS](https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html).
* [rgbguy's framebuffer picking guide](https://rgbguy.in/blogs/object-picking.html) for entity picking.

Assets: the textures in the demo project were drawn in GIMP by me, and the music was also made by me.

Big thanks to all of these great open-source projects and resources for making this learning project possible :)

## Documentation

Full documentation lives
in [docs/](docs/index.md): [build instructions](docs/build.md), [editor guide](docs/editor/usage.md),
[architecture notes](docs/architecture.md), the [ObSL scripting guide](docs/scripting/getting-started.md) and
[API reference](docs/scripting/api-reference.md), and [file format specs](docs/formats/project-json.md).

Licensed under the MIT License. See [LICENSE](LICENSE).

