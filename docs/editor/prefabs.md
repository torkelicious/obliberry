# Prefabs

A **prefab** is a reusable entity template. Create it once, instantiate it many times - across scenes, from scripts, or
in the editor.

---

## Why Prefabs?

* **Reuse** - Define an enemy once, spawn it as much as you want, and where you want.
* **Consistency** - Change the prefab, all instances update (when re-instantiated)
* **Script spawning** - `Instantiate("assets/prefabs/enemy.json")` in ObSL
* **Hierarchy** - Prefabs can contain child entities

---

## Creating a Prefab

1. Set up an entity (or hierarchy) in the Registry exactly how you want it
2. Right-click the **root entity** → **Save as Prefab**
3. Choose a name (e.g., `enemy_basic.json`) - saved to `assets/prefabs/`

---

## Prefab Contents

A prefab file stores:

* The root entity's components
* All child entities (recursively) with their components
* Relative hierarchy (parent/child relationships)

It does **not** store:

* Scene-specific data (map references, scene properties)
* Runtime state (movement progress, particle timers)

---

## Using Prefabs

### In the Editor

**Project Browser → Prefabs tab** → Drag a prefab into the Scene View or Registry.

This creates a new entity (or hierarchy) with the same components. The new entity has a `PrefabSourceComponent` linking
back to the prefab file (editor-only, for "Select Prefab Source" context menu).

### In Scripts (ObSL)

```obsl
// Spawn a single instance
var entity = Instantiate("assets/prefabs/enemy_basic.json");

// Spawn at a specific position
var entity = Instantiate("assets/prefabs/enemy_basic.json");
var tf = entity.GetComponent("Transform");
tf.SetPosition(x, y, z);

```

The `Instantiate` function returns an entity object (or `nil` on failure). Set position via the Transform component
after instantiating.

---

## Prefab File Format

Prefabs use the same entity serialization as scenes (see [scene-json.md](../formats/scene-json.md#entities)), just
without the `properties`, `assets`, `grid`, and `ui` sections.

Example `assets/prefabs/enemy_basic.json`:

```json
[
  {
    "name": "Enemy",
    "components": {
      "TransformComponent": {
        "position": [0, 0, 0],
        "rotation": [0, 0, 0],
        "scale": [1, 1, 1]
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
        "scriptPaths": ["assets/scripts/EnemyAI.obsl"]
      }
    }
  }
]
```

---
