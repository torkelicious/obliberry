# Prefabs

A **prefab** is a reusable entity template. Create it once, instantiate it many times - across scenes, from scripts, or
in the editor.

---

## Why Prefabs?

- **Reuse** - Define an enemy once, spawn it as much as you want, and where you want.
- **Consistency** - Change the prefab, all instances update (when re-instantiated)
- **Script spawning** - `Instantiate("assets/prefabs/enemy.json")` in ObSL
- **Instances** - Each instance gets its own entity UUID

---

## Creating a Prefab

1. Set up an entity in the Registry exactly how you want it
2. Select it and click **Save as Prefab** in the Inspector
3. Enter a name (e.g., `enemy_basic`) and click **Save** - saved to `assets/prefabs/enemy_basic.json`

---

## Prefab Contents

A prefab file stores:

- The entity's name and UUID
- Its serialized component settings

It does **not** store:

- Child entities or hierarchy
- Scene-specific data (map references, scene properties)
- Runtime state (movement progress, live particles, CustomData)

---

## Using Prefabs

### In the Editor

Use **Spawn Prefab** in the Registry panel to choose a file from `assets/prefabs/`.

This creates one entity with the saved components and a new UUID. The Inspector shows its prefab path and controls:

- **Revert** - Replace the instance with a fresh copy of the prefab, discarding edits and removing its children.
- **Break Prefab** - Remove the prefab link while keeping the entity.

Prefab links are not saved in scene files. Editing a prefab does not automatically update existing instances.

### In Scripts (ObSL)

```obsl
// Spawn at a specific position
var entity = Instantiate("assets/prefabs/enemy_basic.json");
if (entity != null) {
    var tf = entity.GetComponent("Transform");
    if (tf != null) {
        tf.SetPosition(2, 0, 0);
    }
}
```

The `Instantiate` function returns an entity object (or `null` on failure). Set position via the Transform component
after instantiating. The prefab's resources must already be loaded in the current scene.

---

## Prefab File Format

Prefabs use the same per-entity serialization as scenes (see
[scene-json.md](../formats/scene-json.md#entities)), without the scene-level `properties`, `grid`, `PostProcessing`, or
`ui` sections. Asset fields contain IDs whose definitions live in the project-wide
[`assets.json`](../formats/assets-json.md); prefab files do not embed asset definitions.
The file contains a single entity object. Its UUID is replaced with a new one when instantiated.

Example `assets/prefabs/enemy_basic.json`:

```json
{
    "uuid": "5a816409-bd9a-488b-90c4-9c78da410d26",
    "name": "Enemy",
    "components": {
        "TransformComponent": {
            "position": [
                0,
                0,
                0
            ],
            "rotation": [
                0,
                0,
                0
            ],
            "scale": [
                1,
                1,
                1
            ]
        },
        "MeshComponent": {
            "mesh_id": "[Engine] Quad"
        },
        "MaterialComponent": {
            "material_id": "enemy_mat"
        },
        "MovementComponent": {
            "timePerStep": 0.5,
            "autoMove": true
        },
        "ScriptComponent": {
            "scriptPaths": [
                "assets/scripts/EnemyAI.obsl"
            ]
        }
    }
}
```

---
