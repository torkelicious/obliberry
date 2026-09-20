# `assets.json` : Project Asset Catalog

`assets.json` lives at the project root beside `project.json`. It is the single catalog of user-defined asset IDs and
the data needed to load them. Scene, prefab, UI, map, post-processing, and animation files store asset IDs; they do not
duplicate asset definitions.

The editor requires a valid catalog before opening a project. New-project templates include an empty catalog. Older
projects that stored definitions inside scene files must be migrated as described
in [Migrating old projects](#migrating-old-projects).

## Top-level structure

```json
{
    "version": 1,
    "assets": {
        "textures": [],
        "shaders": [],
        "meshes": [],
        "materials": [],
        "fonts": [],
        "animation_sets": []
    }
}
```

| Key       | Type    | Meaning                                             |
|-----------|---------|-----------------------------------------------------|
| `version` | integer | Catalog format version. The current version is `1`. |
| `assets`  | object  | Contains the six supported asset-definition arrays. |

All six arrays are normalized into the in-memory catalog even when an array is absent from an older version-1 file.
Unknown asset categories, invalid IDs, and unsupported catalog versions are rejected.

Every entry requires a non-empty string `id`. IDs must be unique within their asset category. Engine-provided IDs,
including values beginning with `[Engine]` and `[Engine_PP]`, are registered internally and are not written here.

## Asset definitions

### Textures

```json
{
    "id": "player_sheet",
    "path": "assets/textures/player.png"
}
```

`path` is a project-relative VFS path.

### Shaders

```json
{
    "id": "lit_sprite",
    "vertex": "assets/shaders/sprite.vert",
    "fragment": "assets/shaders/sprite.frag"
}
```

Regular shaders provide both stages. A post-processing shader normally has an ID beginning with `[PP]` and an empty
`vertex` value; the engine supplies its fullscreen vertex stage.

### Meshes

Procedural meshes store a registered factory name:

```json
{
    "id": "player_mesh",
    "factory": "Quad"
}
```

Custom meshes use the `Custom` factory and additionally store vertex and index data:

```json
{
    "id": "custom_triangle",
    "factory": "Custom",
    "vertices": [
        { "position": [0.0, 0.5, 0.0], "uv": [0.5, 1.0] },
        { "position": [-0.5, -0.5, 0.0], "uv": [0.0, 0.0] },
        { "position": [0.5, -0.5, 0.0], "uv": [1.0, 0.0] }
    ],
    "indices": [0, 1, 2]
}
```

### Materials

```json
{
    "id": "player_material",
    "shader": "[Engine] Base",
    "texture": "player_sheet",
    "color": [1.0, 1.0, 1.0, 1.0]
}
```

`shader` and `texture` are resource IDs. An empty texture ID means no texture. When a material is required, its shader
and texture are treated as dependencies and loaded first.

### Fonts

```json
{
    "id": "dialogue_font",
    "path": "assets/fonts/dialogue.ttf",
    "size": 18,
    "sdf": true,
    "spread": 8
}
```

Defaults are `size: 12`, `sdf: false`, and `spread: 8`.

### Animation sets

```json
{
    "id": "player_animations",
    "path": "assets/animations/player.json"
}
```

The referenced animation JSON contains its sheet and clips. Its `sheet.texture_id` is resolved as a dependency before
the animation set is loaded. See [Sprite Animation JSON](sprite-animation-json.md).

## Reference and loading model

Asset definitions are not loaded merely because they exist in `assets.json`.

When a scene enters, `SceneAssetLoader` scans the scene for resource IDs used by:

- the grid mesh and tile textures;
- post-processing shaders;
- entity mesh, material, directional-texture, particle-material, sprite-sheet, and animation components;
- UI fonts and textures.

It resolves material and animation dependencies, builds the required catalog subset, and loads only that subset.
The scene retains a `SceneAssetScope`; destroying the scene releases its references and unloads resources that are no
longer retained. Persistent entities reacquire their referenced assets for the destination scene.

Editor selections and supported script setters also acquire catalog assets on demand and attach their scopes to the
current scene. Replacing or deleting a catalog entry marks the loaded resource stale; it is discarded safely during a
scene transition after the old scene releases its scopes.

## Editing and saving

The Project Browser creates, updates, and removes definitions through `IO::AssetCatalog`. Catalog writes are atomic on
Linux, macOS, and Windows. Saving a scene writes only its asset IDs. It does not rewrite asset definitions or save an
unfinished sprite-animation draft.

## Migrating old projects

Build the tools with `BUILD_PACK_TOOLS=ON`, then run:

```text
ob_asset_migrator <project-directory> [--dry-run] [--strip-scenes]
```

- With no flags, the tool merges definitions from all `assets/scenes/*.json` files into the project-root
  `assets.json` and preserves the legacy scene sections.
- `--dry-run` validates and reports without writing.
- `--strip-scenes` also removes the legacy `assets` objects from scene files.
- Before overwriting an existing catalog or stripping scenes, the tool creates an
  `asset-migration-backup-*` directory in the project root.
- Conflicting definitions that use the same category and ID abort the migration instead of silently choosing one.

Use `--strip-scenes` after reviewing a dry run when the goal is to remove the old duplicated asset data:

```bash
ob_asset_migrator "/path/to/project" --dry-run
ob_asset_migrator "/path/to/project" --strip-scenes
```

PowerShell:

```powershell
ob_asset_migrator.exe "C:\path\to\project" --dry-run
ob_asset_migrator.exe "C:\path\to\project" --strip-scenes
```

## Related formats

- [Project configuration](project-json.md)
- [Scene JSON](scene-json.md)
- [Sprite Animation JSON](sprite-animation-json.md)
- [Package format and tools](obpak.md)
