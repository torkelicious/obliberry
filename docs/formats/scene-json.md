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

| Key          | Type   | Meaning                                                                                               |
|--------------|--------|-------------------------------------------------------------------------------------------------------|
| `properties` | object | Scene metadata (name, clear color, music, ambient light).                                             |
| `assets`     | object | Asset registry: textures/shaders/meshes/materials/fonts referenced by id from entities, grid, and UI. |
| `grid`       | object | Hex-grid map section; present iff the scene has a map entity with a non-empty `map_file`.             |
| `entities`   | array  | The scene's entities.                                                                                 |
| `ui`         | object | UI element tree (`ui.elements`).                                                                      |

## `properties`

| Key                | Type                  | Default        | Meaning                                                           |
|--------------------|-----------------------|----------------|-------------------------------------------------------------------|
| `name`             | string                | `""`           | Scene display name.                                               |
| `clear_color`      | `[r, g, b, a]` floats | `[0, 0, 0, 1]` | Background clear color.                                           |
| `background_music` | string                | `""`           | VFS-relative path to background music.                            |
| `ambient_light`    | float                 | `0.2`          | Ambient light intensity (also fed into the map lightmap on load). |

## `assets`

Each array entry is an object keyed by `id` the resource id referenced everywhere else in the file. Engine-internal
resources use ids prefixed `"[Engine]"` (e.g. `"[Engine] Base"`, `"[Engine] Hex"`); user assets use any other id.

| Key                | Entry shape                               | Notes                                                                                                                     |
|--------------------|-------------------------------------------|---------------------------------------------------------------------------------------------------------------------------|
| `assets.textures`  | `{"id", "path"}`                          | `path` is VFS-relative.                                                                                                   |
| `assets.shaders`   | `{"id", "vertex", "fragment"}`            | `vertex`/`fragment` are shader source paths. Only non-`[Engine]` shaders are serialized.                                  |
| `assets.meshes`    | `{"id", "factory"}`                       | `factory` names a registered procedural mesh factory (e.g. `"Quad"`, `"Hexagon"`, `"Circle"`, `"Ring"`, `"PointTopHex"`). |
| `assets.materials` | `{"id", "shader", "texture", "color"}`    | `shader` defaults to `"[Engine] Base"`; `texture` may be `""`; `color` is `[r,g,b,a]` (defaults to white).                |
| `assets.fonts`     | `{"id", "path", "size", "sdf", "spread"}` | Defaults: `size` 12, `sdf` false, `spread` 8.                                                                             |

## `grid`

| Key        | Type   | Default          | Meaning                                                                                                                                                                                         |
|------------|--------|------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `map_file` | string | -                | Path to the binary hex map (`.obmap`); see [the `.obmap` format](obmap.md).                                                                                                                     |
| `mesh_id`  | string | `"[Engine] Hex"` | Mesh resource id used for hex cells.                                                                                                                                                            |
| `types`    | array  | -                | Material mapping per tile type id. Each entry: `id` (uint, default 1), `texture` (texture resource id, default `"hex_tex"`), `color` (`[r,g,b,a]`, default white; only written when not white). |

On load, the map becomes a dedicated entity named `"MAP"` with `MapComponent` + `MapStateComponent`; selection and path
overlays are built from the same shader.

## `entities`

Each entity is an object:

| Key          | Type             | Meaning                                                                                                                                                     |
|--------------|------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `name`       | string, optional | Entity name (written only if non-empty).                                                                                                                    |
| `parent`     | int, optional    | **Index** into the same `entities` array identifying the parent (hierarchy is rebuilt after all entities load). Written only if the parent is in the scene. |
| `components` | object           | Maps component type names to per-component data. Unknown component names are skipped with a warning.                                                        |

### Component keys

| Component key                 | Fields                                                                                                                                                                                                                                                                                                                                                                                                        |
|-------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `TransformComponent`          | `position`, `rotation`, `scale` - `[x, y, z]` float arrays (Euler rotation).                                                                                                                                                                                                                                                                                                                                  |
| `MovementComponent`           | `timePerStep` - float; `autoMove` - bool (default `false`). (Runtime movement state is intentionally not saved.)                                                                                                                                                                                                                                                                                                                                   |
| `MeshComponent`               | `mesh_id` - mesh resource id.                                                                                                                                                                                                                                                                                                                                                                                 |
| `MaterialComponent`           | `material_id` - material resource id.                                                                                                                                                                                                                                                                                                                                                                         |
| `BillboardTagComponent`       | `{}` - no fields.                                                                                                                                                                                                                                                                                                                                                                                             |
| `DirectionalTextureComponent` | `index` - int; `textures` - array of exactly 6 texture resource ids (`""` for unset slots).                                                                                                                                                                                                                                                                                                                   |
| `PointLightComponent`         | `color` - `[r, g, b]`; `radius` - float; `intensity` - float.                                                                                                                                                                                                                                                                                                                                                 |
| `ScriptComponent`             | `scriptPath` (string, single script) **or** `scriptPaths` (array, multiple scripts). Paths are `.obsl` files.                                                                                                                                                                                                                                                                                                 |
| `ParticleEmitterComponent`    | `maxParticles` int, `emitRate` float, `lifetimeMin`/`lifetimeMax` floats, `velocityMin`/`velocityMax` `[x,y,z]`, `gravity` `[x,y,z]`, `sizeStartMin`/`sizeStartMax`/`sizeEndMin`/`sizeEndMax` floats, `rotationSpeedMin`/`rotationSpeedMax` floats, `colorStart`/`colorEnd` `[r,g,b,a]`, `isBillboard` bool, `blendMode` int (0 = Alpha, 1 = Additive), `renderOrder` int, `shape` int, `material_id` string. |

**Prefabs** (`PrefabSourceComponent`) are *not* serialized into scene files - they are separate JSON files under
`assets/prefabs/`, written by `IO::PrefabManager::SavePrefab` using the same per-entity structure.
`Instantiate("assets/prefabs/foo.json")` loads one from a script.

## `ui`

`ui.elements` is an array of element objects. Shared fields:

| Key             | Type            | Meaning                                                              |
|-----------------|-----------------|----------------------------------------------------------------------|
| `name`          | string          | Element name (load default `"Unnamed"`).                             |
| `type`          | string          | `"Text"`, `"Button"`, `"Image"`, `"Rect"`, or `"Element"` (default). |
| `rect.position` | `[x, y]` floats | UI-space position.                                                   |
| `rect.scale`    | `[x, y]` floats | UI-space size.                                                       |
| `flags`         | int bitmask     | Bit 0 (`1`) = `VISIBLE`, bit 1 (`2`) = `ENABLED`.                    |
| `children`      | array           | Recursive list of child elements.                                    |

Type-specific fields:

* `Text`: `text`, `color` (`[r,g,b,a]`), `font` (font resource id).
* `Button`: `text`, `color`, `bg_color`, `hovered_bg_color` (`[r,g,b,a]`), `bg_texture` (texture resource id, optional),
  `font` (optional).
* `Image`: `texture` (texture resource id), `color`.
* `Rect`: `color`.

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
      { "id": "dirt_tex", "path": "assets/textures/HexDirt.png" },
      { "id": "grass_tex", "path": "assets/textures/HexGrass.png" }
    ],
    "materials": [
      { "id": "[Engine] DefaultMaterial", "shader": "[Engine] Base", "texture": "", "color": [1.0, 1.0, 1.0, 1.0] }
    ],
    "meshes": [
      { "id": "[Engine] Hex", "factory": "PointTopHex" },
      { "id": "player_mesh", "factory": "Quad" }
    ],
    "shaders": [],
    "fonts": []
  },
  "grid": {
    "map_file": "assets/maps/level1.obmap",
    "mesh_id": "[Engine] Hex",
    "types": [
      { "id": 0, "texture": "sand_tex" },
      { "id": 1, "texture": "grass_tex" }
    ]
  },
  "entities": [
    {
      "name": "Player",
      "components": {
        "TransformComponent": { "position": [0.0, 0.0, 0.0], "rotation": [0.0, 0.0, 0.0], "scale": [1.0, 1.0, 1.0] },
        "MeshComponent": { "mesh_id": "player_mesh" },
        "MaterialComponent": { "material_id": "[Engine] DefaultMaterial" },
        "ScriptComponent": { "scriptPath": "assets/scripts/PlayerMovement.obsl" }
      }
    }
  ],
  "ui": {
    "elements": [
      {
        "name": "btnStart",
        "type": "Button",
        "rect": { "position": [160.0, 270.0], "scale": [251.0, 59.0] },
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

* Maps are stored as `.obmap` binaries, referenced by `map_file`; the grid is *not* embedded in the scene JSON.
* `PrefabSourceComponent` is editor metadata only and never appears in scene files.
* `properties.name` is a display name; the file path is the real scene identity (`ScenePath` is runtime-only and never
  serialized).
