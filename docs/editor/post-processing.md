# Post-Processing

Post-processing applies a chain of fullscreen shader effects to the rendered scene

Post Processing chain is stored **per scene**, inside the scene JSON (see
[scene format](../formats/scene-json.md#postprocessing)).

---

## Post Processing Window

**Window → Post Processing**.

### effect chain

The top list shows the effect chain. Effects run **top to bottom**; each effect's output is the next effect's input.

* **Checkbox** - Enable/disable the effect without removing it.
* **Click** an effect to select it and edit its settings below.
* **Drag & drop** an entry to reorder the chain.
* **Add effect...** - Adds one of the built-in effects, or use **Import shader...** to bring in your own.
* **Delete** - Removes the selected effect from the chain.

Reordering, enabling, adding, and deleting take effect immediately in the Scene View

### Effect settings

* **Passes (1–16)** - How many times the effect runs, ping-ponging between render targets. Multi-pass effects like a
  separable blur need one pass per direction.
* **Uses scene texture (`u_Scene`)** - Also binds the *original* unprocessed scene on texture unit 1, for compositing
  effects (e.g. bloom adds the blurred bright-pass back onto the scene).
* **Uniforms** - Editable values per effect. `float`/`int`/`vec2/3/4` get drag widgets, uniforms with `Color` in the
  name get a color picker. **Ctrl+click** a drag widget to type an exact value.
* **Pass N** sections - Per-pass uniform overrides for multi-pass effects (e.g. `u_Horizontal` for the blur).

### Live preview, Apply, Close

Edits are live-previewed in the Scene View as you make them but they are not committed until you press **Apply** or
**Save & Close**:

Committing marks the scene as changed remember to save the scene (Ctrl+S) afterwards to persist it.

---

## Built-in Effects

| Effect             | does                                                                | uniforms                                                                                              |
|--------------------|---------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------|
| **Passthrough**    | Copies the input unchanged                                          | –                                                                                                     |
| **Grayscale**      | Desaturates the image                                               | `u_Strength` (0–1)                                                                                    |
| **BrightPass**     | Extracts pixels brighter than a threshold                           | `u_Threshold`, `u_SoftKnee` (soft cutoff)                                                             |
| **GaussianBlur**   | 9-tap separable blur; needs 2 passes, `u_Horizontal` 1 then 0       | `u_Horizontal` (per-pass, 1/0)                                                                        |
| **BloomComposite** | Adds the blurred bright-pass back onto the scene                    | `u_Strength`                                                                                          |
| **CRT**            | Retro CRT monitor: curvature, scanlines, mask, glow, noise, flicker | `u_Curvature`, `u_Aberration`, `u_Scanline`, `u_Mask`, `u_Glow`, `u_Noise`, `u_Flicker`, `u_Vignette` |

A default chain containing all of these (disabled by default) is used for scenes that don't specify one.

---

## Custom Effects

Use **Add effect... → Import shader...** and pick a `.frag`/`.glsl` / `.shader` file. It's copied into
`assets/shaders/`, its non-reserved uniforms are parsed automatically, and a new (disabled) effect is added to the
chain.

Effect shaders share a fixed vertex stage (a fullscreen triangle), so you only write a fragment shader:

```glsl
#version 330 core
in vec2 v_UV;              // fullscreen UVs provided by the engine
out vec4 FragColor;

uniform sampler2D u_Texture;   // previous pass output
uniform float u_Strength;      // this is picked up automatically for the editor UI

void main() {
    vec3 c = texture(u_Texture, v_UV).rgb;
    FragColor = vec4(c * u_Strength, 1.0);
}
```

### Engine uniforms

The engine sets these for every effect when the shader declares them. don't declare your own values for them in the
effect config, they're automatically ser per-frame:

| Uniform        | Type        | Meaning                                             |
|----------------|-------------|-----------------------------------------------------|
| `u_Texture`    | `sampler2D` | Previous pass output (texture unit 0).              |
| `u_Resolution` | `vec2`      | Render target size in pixels.                       |
| `u_TexelSize`  | `vec2`      | `1.0 / u_Resolution`.                               |
| `u_Time`       | `float`     | Seconds since engine start.                         |
| `u_Scene`      | `sampler2D` | Original scene (unit 1); only with *scene texture*. |

### Uniform rules

* Supported types for editing/serialization: `float`, `int`, `bool`, `vec2`, `vec3`, `vec4` (`bool` is stored as
  `int` 0/1). Samplers and matrices are not currently exposed.
* Reserved names (`u_Texture`, `u_Resolution`, `u_TexelSize`, `u_Time`, `u_Scene`) are managed by the engine and
  excluded from the editor UI.
* The parser is simple: no arrays, no layout qualifiers etc.

Shader files go through the engine's preprocessor, so `#include "..."`, `#pragma once`, and accurate `#line` error
reporting all work in effect shaders too.

* Imported shaders get the resource ID `[PP] <name>` and are regenerated from the scene's `assets.shaders` section on
  load. A shader entry with **no vertex path** is automatically treated as a post-processing effect shader and gets the
  fullscreen vertex pass.
* Always make sure the scene referencing an effect is saved after importing, so the chain + shader asset both persist.

---

## See Also

* [Scene format: PostProcessing](../formats/scene-json.md#postprocessing)
* [Scenes](scenes.md)
* [Architecture](../architecture.md#rendering-srcrendering) 
