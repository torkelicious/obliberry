# `graphics.json` - Graphics & Window Configuration

`graphics.json` lives at the **root of a project**, next to `project.json`. It controls window and rendering settings
and is read by both the editor and the runtime.

## Loading

* Deserialized by `Config::GraphicsConfig::Deserialize("graphics.json")` right after the VFS mount (runtime) or after
  `Core::Project::Load` (editor).
* Read order: VFS first, then a fallback to the loose file `<project root>/graphics.json`. If neither exists, the
  defaults below are used with a warning.
* Unlike `project.json` and scene files, `graphics.json` is **always parsed as plain text JSON** even inside a packaged
  `.obpak`. A packaged graphics config must be text JSON, otherwise all defaults apply.
* The editor writes it back via `Config::GraphicsConfig::Serialize`.

## Fields

| Key                    | Type           | Default      | Meaning                                                                                                                                                               |
|------------------------|----------------|--------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `window`               | object         | -            | Window settings container.                                                                                                                                            |
| `window.width`         | int            | `1280`       | Initial window width in pixels.                                                                                                                                       |
| `window.height`        | int            | `720`        | Initial window height in pixels.                                                                                                                                      |
| `window.fullscreen`    | bool           | `false`      | Start in fullscreen.                                                                                                                                                  |
| `antialiasing`         | object         | -            | MSAA settings container.                                                                                                                                              |
| `antialiasing.MSAA`    | bool           | `false`      | Master MSAA enable flag.                                                                                                                                              |
| `antialiasing.samples` | int            | `4`          | Sample count used **if MSAA is on**. Snapped to the nearest supported GL sample count (`{1, 2, 4, 8, 16}` filtered by `GL_MAX_SAMPLES`).                              |
| `targetfps`            | int            | `60`         | Target FPS for the frame limiter (applied on the render thread when VSync is off).                                                                                    |
| `vsync`                | string or bool | `"standard"` | VSync mode. Strings: `"none"`, `"standard"`, `"adaptive"`. A boolean is also accepted: `true` → `"standard"`, `false` → `"none"`. Always serialized back as a string. |
| `overlay`              | bool           | `false`      | Show the performance overlay - **runtime only**.                                                                                                                      |

## Example

```json
{
  "window": {
    "width": 1280,
    "height": 720,
    "fullscreen": false
  },
  "antialiasing": {
    "MSAA": false,
    "samples": 4
  },
  "targetfps": 60,
  "vsync": "standard",
  "overlay": false
}
```

## Notes

* `overlay` is optional on load and omitted from the shipped template; it is runtime-only.
* The `samples` value is snapped to the nearest valid sample count both when applied and when re-serialized, so a value
  like `3` won't round-trip unchanged.
* VSync enum values match GLFW: `ADAPTIVE = -1`, `NONE = 0`, `STANDARD = 1`.
