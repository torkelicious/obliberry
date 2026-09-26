# `project.json` : Project Configuration

`project.json` sits at the **root of a project** and defines the project's identity, startup scene, window title, and
save-game settings. The project root is the directory containing this file.

A valid project also contains a project-root [`assets.json`](assets-json.md) catalog. `project.json` selects the start
scene; `assets.json` maps the resource IDs used by that and other scenes to their load definitions.

## Loading

- **Runtime** (`src/Applications/Runtime/RuntimeMain.cpp`): mounts a project (`-p`/`--project <path>` or autodetected
  `project.json` in the working directory) or a package (`-pk`/`--package <path>` / `data.obpak`), then reads
  `project.json` through the VFS.
- **Editor**: `Core::Project::Load(projectFilePath)` mounts the project and loads the config; `Project::NewProject`
  copies a template and assigns the new project its own title and UUID.
- Parsing: read via the VFS. Inside a packaged `.obpak`, the file is expected to be a **msgpack-encoded** JSON blob;
  otherwise it is parsed as plain text JSON. Missing or invalid optional fields keep their defaults.

## Fields

| Key              | Type    | Default               | Meaning |
|------------------|---------|-----------------------|---------|
| `UUID`           | string  | `""`                  | Stable project identifier. New projects use a UUID v4. The value separates this project's save directory from other projects. |
| `window`         | object  | -                     | Window settings container. |
| `window.title`   | string  | `"Obliberry Project"` | Window/application title. The runtime uses it directly; the editor shows `"Obliberry: <title>"`. |
| `start_scene`    | string  | `""`                  | VFS-relative scene path loaded at startup, for example `"assets/scenes/default.json"`. |
| `saves`          | object  | -                     | Save-game settings container. |
| `saves.enabled`  | boolean | `false`               | Configures persistent save-game storage for the project. |
| `saves.location` | integer | `0`                   | Storage mode: `0` = operating-system data directory, `1` = portable storage beside the executable. |

## Example

```json
{
  "UUID": "6f7dcf16-2f23-46de-b95f-f49d797a24ac",
  "window": {
    "title": "My Game"
  },
  "start_scene": "assets/scenes/default.json",
  "saves": {
    "enabled": true,
    "location": 0
  }
}
```

## Project UUID

The UUID identifies a project independently of its name or directory. Save paths contain this UUID, so two projects
with the same title do not share save files.

Regenerating the UUID changes where the project looks for saves. Existing save files are not deleted, but they remain
under the previous UUID and are no longer discovered automatically.

## Save locations

With `saves.location` set to `0`, saves are stored below the platform data directory:

- **Windows:** `%LOCALAPPDATA%/obliberry/<project UUID>/saves`
- **Linux:** `$XDG_DATA_HOME/obliberry/<project UUID>/saves`
- **macOS:** `~/Library/Application Support/obliberry/<project UUID>/saves`

With `saves.location` set to `1`, saves are stored at:

```text
<executable directory>/obliberry/<project UUID>/saves
```

Portable storage requires the executable directory to be writable.

See [Save-game format](save-json.md) for filenames, JSON fields, and the script-facing lifecycle.

## Writing

The editor serializes `UUID`, `window.title`, `start_scene`, `saves.enabled`, and `saves.location` when the project
configuration is saved.

## Notes

- Window size, VSync, MSAA, and other graphics settings live in [graphics.json](graphics-json.md).
- Because the VFS mounts at the directory containing `project.json`, project asset paths are VFS-relative.
- `Project::GetActive()` exposes the loaded project; the editor tracks unsaved configuration changes on it.
