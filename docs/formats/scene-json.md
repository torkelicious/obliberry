# Scene Files (`assets/scenes/*.json`)

Scenes describe everything in a Scene: properties, assets, the hex grid, entities, and UI. They live under
`assets/scenes/` inside a project and are referenced by `project.json`'s `start_scene`.

I/O is handled by `IO::SceneIO` (`src/IO/SceneSerialization.cpp`). Loading order matters: `properties` → `assets` (asset
factories must be registered first) → `grid` → `entities` (with `parent` re-parenting applied after all entities
exist) → `ui`.

When saving: entities with a `MapComponent` are **not** written as entities (they become the `grid` section), floats are
rounded to 3 decimals (`RoundJsonFloats`), and the file is written with 4-space indentation.

> **Packaged builds:** inside a `.obpak`, scene files are stored as **msgpack-encoded** JSON blobs (`BinaryJSON`
> entries) and decoded on load.

## Top-level keys

| Key              | Type   | Meaning                                                                                               |
| ---------------- | ------ | ----------------------------------------------------------------------------------------------------- |
| `properties`     | object | Scene metadata (name, clear color, music, ambient light).                                             |
| `assets`         | object | Asset registry: textures/shaders/meshes/materials/fonts referenced by id from entities, grid, and UI. |
| `grid`           | object | Hex-grid map section; present iff the scene has a map entity with a non-empty `map_file`.             |
| `PostProcessing` | array  | Post-processing effect chain; absent means the default built-in chain (all effects disabled).         |
| `entities`       | array  | The scene's entities.                                                                                 |
| `ui`             | object | UI element tree (`ui.elements`).                                                                      |

## `properties`

| Key                | Type                  | Default        | Meaning                                                           |
| ------------------ | --------------------- | -------------- | ----------------------------------------------------------------- |
| `name`             | string                | `""`           | Scene display name.                                               |
| `clear_color`      | `[r, g, b, a]` floats | `[0, 0, 0, 1]` | Background clear color.                                           |
| `background_music` | string                | `""`           | VFS-relative path to background music.                            |
| `ambient_light`    | float                 | `0.2`          | Ambient light intensity (also fed into the map lightmap on load). |

## `assets`

Each array entry is an object keyed by `id` the resource id referenced everywhere else in the file. Engine-internal
resources use ids prefixed `"[Engine]"` (e.g. `"[Engine] Base"`, `"[Engine] Hex"`); user assets use any other id.

| Key                       | Entry shape                                                                                                                                                                                   | Notes                                                                                                                                                                                                                                                                            |
| ------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `assets.textures`         | `{"id", "path"}`                                                                                                                                                                              | `path` is VFS-relative.                                                                                                                                                                                                                                                          |
| `assets.shaders`          | `{"id", "vertex", "fragment"}`                                                                                                                                                                | `vertex`/`fragment` are shader source paths. `[Engine]` and `[Engine_PP]` shaders are engine-provided and never serialized. A shader with an empty `vertex` (or a `[PP]` id) is a **post-processing effect shader** (it automatically uses the engine's fullscreen vertex pass). |
| `assets.meshes`           | `{"id", "factory"}`                                                                                                                                                                           | `factory` names a registered procedural mesh factory (e.g. `"Quad"`, `"Hexagon"`, `"Circle"`, `"Ring"`, `"PointTopHex"`).                                                                                                                                                        |
| `assets.materials`        | `{"id", "shader", "texture", "color"}`                                                                                                                                                        | `shader` defaults to `"[Engine] Base"`; `texture` may be `""`; `color` is `[r,g,b,a]` (defaults to white).                                                                                                                                                                       |
| `assets.fonts`            | `{"id", "path", "size", "sdf", "spread"}`                                                                                                                                                     | Defaults: `size` 12, `sdf` false, `spread` 8.                                                                                                                                                                                                                                    |
| `SpriteSheetComponent`    | Static sprite: `texture_id`, `columns`, `rows`, `column_spacing`, `row_spacing`, and `frame`. For an entity with SpriteAnimator, saved as `{}` because its pose is derived from the animator. |
| `SpriteAnimatorComponent` | `animation_id` references an entry in `assets.animation_sets`; `initial_clip` selects the initial clip; `autoplay` controls initial playback.                                                 |

### Sprite animation references

Animation definitions live in separate
[sprite animation JSON files](sprite-animation-json.md).
The scene records their resource IDs and paths:

```json
{
    "animation_sets": [
        {
            "id": "player_animations",
            "path": "assets/animations/player_anim.json"
        }
    ]
}
```

The object above belongs inside the scene's `assets` object.

An animated entity uses these entries inside its `components` object:

```json
{
    "SpriteSheetComponent": {},
    "SpriteAnimatorComponent": {
        "animation_id": "player_animations",
        "initial_clip": "idle",
        "autoplay": true
    }
}
```

The resource ID is separate from the file path.

The scene saves the initial playback configuration, not the current
clip position, elapsed time, or playing/paused state. Loading initializes
the animator and resolves its initial sprite pose.

Saving the scene records animation asset references. Save edited clip
definitions from the animation editor as well.

## `PostProcessing`

The post-processing effect chain as an ordered JSON array (first entry runs first). Written back by the editor's **Post
Processing** window; see [Post-Processing](../editor/post-processing.md).

Each entry is an object:

| Key                 | Type   | Default  | Meaning                                                                                   |
| ------------------- | ------ | -------- | ----------------------------------------------------------------------------------------- |
| `shader`            | string | required | Shader resource id. Built-ins are `[Engine_PP] <name>`; custom effects are `[PP] <name>`. |
| `enabled`           | bool   | `true`   | Disabled effects are skipped.                                                             |
| `uniforms`          | object | `{}`     | Tunable uniforms. Values: number (float/int), bool, or `[x,y(,z,w)]` arrays.              |
| `passes`            | int    | `1`      | How many times the effect runs (ping-ponging). Only written when > 1.                     |
| `passUniforms`      | array  | `[]`     | Per-pass uniform overrides, same shape as `uniforms`, one object per pass.                |
| `wantsSceneTexture` | bool   | `false`  | Also bind the original scene on unit 1 as `u_Scene` (for compositing effects like bloom). |

Example (bloom chain):

```json
"PostProcessing": [
    {
        "shader": "[Engine_PP] BrightPass",
        "enabled": true,
        "uniforms": {
            "u_Threshold": 0.8,
            "u_SoftKnee": 0.5
        }
    },
    {
        "shader": "[Engine_PP] GaussianBlur",
        "enabled": true,
        "passes": 2,
        "passUniforms": [
            {
                "u_Horizontal": 1
            },
            {
                "u_Horizontal": 0
            }
        ]
    },
    {
        "shader": "[Engine_PP] BloomComposite",
        "enabled": true,
        "wantsSceneTexture": true,
        "uniforms": {
            "u_Strength": 1.0
        }
    }
]
```

## `grid`

| Key        | Type   | Default          | Meaning                                                                                                                                                                                         |
| ---------- | ------ | ---------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `map_file` | string | -                | Path to the binary hex map (`.obmap`); see [the `.obmap` format](obmap.md).                                                                                                                     |
| `mesh_id`  | string | `"[Engine] Hex"` | Mesh resource id used for hex cells.                                                                                                                                                            |
| `types`    | array  | -                | Material mapping per tile type id. Each entry: `id` (uint, default 1), `texture` (texture resource id, default `"hex_tex"`), `color` (`[r,g,b,a]`, default white; only written when not white). |

On load, the map becomes a dedicated entity named `"MAP"` with `MapComponent` + `MapStateComponent`; selection and path
overlays are built from the same shader.

## `entities`

Each entity is an object:

| Key          | Type             | Meaning                                                                                                                                                     |
| ------------ | ---------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `name`       | string, optional | Entity name (written only if non-empty).                                                                                                                    |
| `parent`     | int, optional    | **Index** into the same `entities` array identifying the parent (hierarchy is rebuilt after all entities load). Written only if the parent is in the scene. |
| `components` | object           | Maps component type names to per-component data. Unknown component names are skipped with a warning.                                                        |

### Component keys

| Component key                 | Fields                                                                                                                                                                                                                                                                                                                                                                                                        |
| ----------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `TransformComponent`          | `position`, `rotation`, `scale` - `[x, y, z]` float arrays (Euler rotation).                                                                                                                                                                                                                                                                                                                                  |
| `MovementComponent`           | `timePerStep` - float; `autoMove` - bool (default `false`). (Runtime movement state is intentionally not saved.)                                                                                                                                                                                                                                                                                              |
| `MeshComponent`               | `mesh_id` - mesh resource id.                                                                                                                                                                                                                                                                                                                                                                                 |
| `MaterialComponent`           | `material_id` - material resource id.                                                                                                                                                                                                                                                                                                                                                                         |
| `BillboardTagComponent`       | `{}` - no fields.                                                                                                                                                                                                                                                                                                                                                                                             |
| `DirectionalTextureComponent` | `index` - int; `textures` - array of exactly 6 texture resource ids (`""` for unset slots).                                                                                                                                                                                                                                                                                                                   |
| `PointLightComponent`         | `color` - `[r, g, b]`; `radius` - float; `intensity` - float.                                                                                                                                                                                                                                                                                                                                                 |
| `ScriptComponent`             | `scriptPath` (string, single script) **or** `scriptPaths` (array, multiple scripts). Paths are `.obsl` files.                                                                                                                                                                                                                                                                                                 |
| `ParticleEmitterComponent`    | `maxParticles` int, `emitRate` float, `lifetimeMin`/`lifetimeMax` floats, `velocityMin`/`velocityMax` `[x,y,z]`, `gravity` `[x,y,z]`, `sizeStartMin`/`sizeStartMax`/`sizeEndMin`/`sizeEndMax` floats, `rotationSpeedMin`/`rotationSpeedMax` floats, `colorStart`/`colorEnd` `[r,g,b,a]`, `isBillboard` bool, `blendMode` int (0 = Alpha, 1 = Additive), `renderOrder` int, `shape` int, `material_id` string. |

### `SpriteSheetComponent`

```json
{
    "SpriteSheetComponent": {
        "texture_id": "hero_walk",
        "columns": 8,
        "rows": 4,
        "column_spacing": 2,
        "row_spacing": 2,
        "start_frame": 8,
        "frame_count": 8,
        "fps": 12.0,
        "looping": true,
        "playing": true,
        "current_frame": 0
    }
}
```

Place this entry inside an entity's `components` object. `texture_id` references a registered
texture resource. This example fits a 270 × 134 texture containing 32 × 32 frames and two-pixel gaps.

| Field                           | Default    | Meaning                                                                            |
| ------------------------------- | ---------- | ---------------------------------------------------------------------------------- |
| `texture_id`                    | No texture | Texture resource key.                                                              |
| `columns`, `rows`               | 1 each     | Grid dimensions.                                                                   |
| `column_spacing`, `row_spacing` | 0 each     | Pixel gaps between frames; negative values clamp to zero on load. No outer margin. |
| `start_frame`                   | 0          | Absolute first sheet frame, numbered from the top-left across rows.                |
| `frame_count`                   | 1          | Clip length, not grid cell count.                                                  |
| `fps`                           | 8.0        | Frames per second.                                                                 |
| `looping`, `playing`            | false each | Playback flags.                                                                    |
| `current_frame`                 | 0          | Current frame relative to `start_frame`.                                           |

Elapsed time within a frame is not saved. The loader does not perform the editor's full layout/clip
validation: author positive grid dimensions, a clip within the grid, and spacing that leaves whole
pixel-sized frames. See [spacing rules](../editor/components.md#spacing-and-invalid-layouts).
The runtime UV helper falls back to the whole texture for invalid texture/grid/spacing geometry.

### `ColliderComponent`

```json
{
    "ColliderComponent": {
        "version": 2,
        "shape": "Box",
        "orientation": "Entity",
        "offset": [0.0, 0.0, 0.0],
        "size": [1.0, 1.0, 1.0],
        "radius": 0.5,
        "height": 1.0,
        "isTrigger": false
    }
}
```

Place this entry inside `components`. `version` must be **2**; missing or older versions raise a
load error. Shape names are case-sensitive: `Box`, `Sphere`, `Cylinder`, `Rectangle`, `Circle`.
Orientation is `Entity` or `Billboard`. Unknown names raise a load error.

The example shows the defaults for all fields except the required version. Offset is local xyz;
size contains full box dimensions (Rectangle uses xy). Sphere/Circle use radius; Cylinder uses
radius and full height along local Y. Rectangle/Circle occupy the local XY plane.

Offset must be finite. The current validator requires all three size entries to be finite and
positive even for shapes that do not use every entry. Radius must be finite and positive for
Sphere, Circle, and Cylinder; Cylinder height must also be finite and positive. Invalid dimensions
raise a load error. All these fields are serialized, including dimensions unused by the selected shape.
`isTrigger` selects trigger events when either collider in an overlapping pair has it enabled.

**Prefabs** (`PrefabSourceComponent`) are _not_ serialized into scene files - they are separate JSON files under
`assets/prefabs/`, written by `IO::PrefabManager::SavePrefab` using the same per-entity structure.
`Instantiate("assets/prefabs/foo.json")` loads one from a script.

## `ui`

`ui.elements` is an array of element objects. Shared fields:

| Key             | Type            | Meaning                                                              |
| --------------- | --------------- | -------------------------------------------------------------------- |
| `name`          | string          | Element name (load default `"Unnamed"`).                             |
| `type`          | string          | `"Text"`, `"Button"`, `"Image"`, `"Rect"`, or `"Element"` (default). |
| `rect.position` | `[x, y]` floats | UI-space position.                                                   |
| `rect.scale`    | `[x, y]` floats | UI-space size.                                                       |
| `flags`         | int bitmask     | Bit 0 (`1`) = `VISIBLE`, bit 1 (`2`) = `ENABLED`.                    |
| `children`      | array           | Recursive list of child elements.                                    |

Type-specific fields:

- `Text`: `text`, `color` (`[r,g,b,a]`), `font` (font resource id).
- `Button`: `text`, `color`, `bg_color`, `hovered_bg_color` (`[r,g,b,a]`), `bg_texture` (texture resource id, optional),
  `font` (optional).
- `Image`: `texture` (texture resource id), `color`.
- `Rect`: `color`.

## Example

A trimmed scene combining most sections:

```json
{
    "properties": {
        "name": "level1",
        "clear_color": [0.1, 0.1, 0.1, 1.0],
        "background_music": "",
        "ambient_light": 0.2
    },
    "assets": {
        "textures": [
            {
                "id": "dirt_tex",
                "path": "assets/textures/HexDirt.png"
            },
            {
                "id": "grass_tex",
                "path": "assets/textures/HexGrass.png"
            }
        ],
        "materials": [
            {
                "id": "[Engine] DefaultMaterial",
                "shader": "[Engine] Base",
                "texture": "",
                "color": [1.0, 1.0, 1.0, 1.0]
            }
        ],
        "meshes": [
            {
                "id": "[Engine] Hex",
                "factory": "PointTopHex"
            },
            {
                "id": "player_mesh",
                "factory": "Quad"
            }
        ],
        "shaders": [],
        "fonts": []
    },
    "grid": {
        "map_file": "assets/maps/level1.obmap",
        "mesh_id": "[Engine] Hex",
        "types": [
            {
                "id": 0,
                "texture": "sand_tex"
            },
            {
                "id": 1,
                "texture": "grass_tex"
            }
        ]
    },
    "entities": [
        {
            "name": "Player",
            "components": {
                "TransformComponent": {
                    "position": [0.0, 0.0, 0.0],
                    "rotation": [0.0, 0.0, 0.0],
                    "scale": [1.0, 1.0, 1.0]
                },
                "MeshComponent": {
                    "mesh_id": "player_mesh"
                },
                "MaterialComponent": {
                    "material_id": "[Engine] DefaultMaterial"
                },
                "ScriptComponent": {
                    "scriptPath": "assets/scripts/PlayerMovement.obsl"
                }
            }
        }
    ],
    "ui": {
        "elements": [
            {
                "name": "btnStart",
                "type": "Button",
                "rect": {
                    "position": [160.0, 270.0],
                    "scale": [251.0, 59.0]
                },
                "flags": 3,
                "text": "Start",
                "color": [1.0, 1.0, 1.0, 1.0],
                "bg_color": [0.5, 0.54, 0.8, 1.0]
            }
        ]
    }
}
```

## Caveats

- Maps are stored as `.obmap` binaries, referenced by `map_file`; the grid is _not_ embedded in the scene JSON.
- `PrefabSourceComponent` is editor metadata only and never appears in scene files.
- `properties.name` is a display name; the file path is the real scene identity (`ScenePath` is runtime-only and never
  serialized).
