# `project.json` : Project Configuration

`project.json` sits at the **root of a project** and defines the project's identity and startup behavior. The project
root is defined as the directory containing this file.

## Loading

* **Runtime** (`src/Applications/Runtime/RuntimeMain.cpp`): mounts a project (`-p`/`--project <path>` or autodetected
  `project.json` in the working directory) or a package (`-pk`/`--package <path>` / `data.obpak`), then reads
  `project.json` through the VFS.
* **Editor**: `Core::Project::Load(projectFilePath)` mounts the project and loads the config; `Project::NewProject`
  copies a template, then overrides `title` with the new project name.
* Parsing: read via the VFS. Inside a packaged `.obpak`, the file is expected to be a **msgpack-encoded** JSON blob;
  otherwise it is parsed as plain text JSON. If the file is missing, the defaults below are used and a warning is
  logged.

## Fields

| Key            | Type   | Default               | Meaning                                                                                                                                                    |
|----------------|--------|-----------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `window`       | object | -                     | Window settings container.                                                                                                                                 |
| `window.title` | string | `"Obliberry Project"` | Window/application title. The runtime uses it directly; the editor shows `"Obliberry: <title>"`.                                                           |
| `start_scene`  | string | `""`                  | VFS-relative path of the scene loaded at startup, e.g. `"assets/scenes/default.json"`. New projects default this to `assets/scenes/default.json` if empty. |

## Example

```json
{
  "start_scene": "assets/scenes/default.json",
  "window": {
    "title": "My Game"
  }
}
```

(The shipped `Templates/DemoProject/project.json` uses exactly this with the title `"DemoProject"`.)

## Writing

The editor writes `project.json` back. Serialization writes only the two keys above, in this order: `start_scene`,
`window.title`.

## Notes

* Only `title` and `start_scene` are read window *size* and graphics settings live in [graphics.json](graphics-json.md).
* Because the VFS mounts at the directory containing `project.json`, all paths inside the file (and in scenes, maps,
  scripts, etc.) are project-relative.
* `Project::GetActive()` (engine-side) exposes the loaded project; the editor tracks unsaved changes on it.
