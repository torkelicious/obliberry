# `graphics.json` - Graphics & Window Configuration

`graphics.json` lives at the **root of a project**, next to `project.json`. It controls window and rendering settings
and is read by both the editor and the runtime.

## Loading

- Loose projects read `graphics.json` from the project root.
- Packaged builds first read text `graphics.json` beside the executable. If it is missing or invalid, the loader
  tries the packaged configuration, then uses defaults. Packaged JSON is stored as msgpack and decoded by the VFS.
- Editor export copies a loose `graphics.json` beside the runtime; it is not part of the export dependency set.
  With the directory-based `ob_packer`, add `!graphics.json` to `.pakignore` to include a packaged fallback.
- **Save** in the editor's Graphics Settings writes the project configuration.

A valid loose configuration replaces the packaged configuration; omitted fields use the defaults below.

## Fields

| Key                    | Type           | Default      | Meaning                                                                                                                                                               |
| ---------------------- | -------------- | ------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
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

- `overlay` is optional on load and omitted from the shipped template; it is runtime-only.
- The `samples` value is snapped to the nearest valid sample count both when applied and when re-serialized, so a value
  like `3` won't round-trip unchanged.
- VSync enum values match GLFW: `ADAPTIVE = -1`, `NONE = 0`, `STANDARD = 1`.
