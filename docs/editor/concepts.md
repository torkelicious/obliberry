# Core Concepts

This page explains the mental model behind Obliberry. Understanding these concepts will make the editor and scripting
make sense.

---

## Entities

An **entity** is just a unique ID. It has no data, no behavior, just an entry in the registry.

* Created via **Registry → +** or `CreateEntity()` in script
* Deleted via **Registry → -** or `DestroyEntity()` (adds `DestroyTag`)
* Can have a **name** (for debugging/editor only)
* Can be **parented** to form hierarchies (handled by `Relationship` component)

---

## Components

A **component** is a plain data structure. Examples: `TransformComponent` (position/rotation/scale), `MeshComponent` (
mesh ID), `ScriptComponent` (list of script paths).

* **One per entity per type** - You can't have two `TransformComponent`s on one entity
* **But components can hold arrays** - One `ScriptComponent` holds *many* script paths
* **Added/removed in Inspector** - Click "Add Component" dropdown, or via script

### Tags vs. Components

Some components are **tags** - empty structs that just mark an entity for a system:

| Tag            | Meaning                                                     |
|----------------|-------------------------------------------------------------|
| `BillboardTag` | Render this entity as a billboard (always faces camera)     |
| `DestroyTag`   | Delete this entity at end of frame                          |
| `Relationship` | Stores parent/child links (not shown in Add Component menu) |

Tags appear as **checkboxes/flags** in the Inspector, not as expandable components.

---

## Systems

**Systems** contain the actual logic. They run every frame (or fixed timestep) and process all entities that have a
specific set of components.

| System                     | Required Components                                  | What It Does                                                             |
|----------------------------|------------------------------------------------------|--------------------------------------------------------------------------|
| `RenderSystem`             | Transform + Mesh + Material                          | Draws the entity                                                         |
| `MovementSystem`           | Transform + Movement                                 | Moves entity along hex path                                              |
| `AISystem`                 | —                                                    | Randomly wanders entities with `autoMove` enabled on `MovementComponent` |
| `DirectionalTextureSystem` | Transform + Movement + DirectionalTexture + Material | Swaps texture based on facing                                            |
| `ScriptSystem`             | Script                                               | Runs ObSL scripts                                                        |
| `ParticleSystem`           | Transform + ParticleEmitter + Material               | Spawns/updates particles                                                 |
| `LightingSystem`           | PointLight                                           | Computes lighting                                                        |
| `UISystem`                 | (UI elements)                                        | Layout, input, rendering                                                 |

> [!NOTE]
> You don't interact with systems directly in the editor. They run automatically in Play mode. In Edit mode, only the
`RenderSystem` runs (for the Scene View).

---

## The Registry

The **Registry** (left panel in Edit mode) is the scene's entity list. It shows:

* Entity name (or "Entity #123" if unnamed)
* Component icons/tags
* Hierarchy (parent/child indentation)

Right-click an entity for:

* **Create Child** - New entity parented to this one
* **Set Parent** - Reparent to another entity
* **Detach** - Remove from parent

---

## Assets & Resources

Assets (textures, meshes, materials, shaders, fonts) are **referenced by ID**, not embedded in entities.

* **Project Browser** (bottom panel) - Browse and create assets
* **Material** - Links a shader + texture + color. Entities reference a Material ID.
* **Mesh** - Procedural shapes (Quad, Hexagon, etc.) or loaded models. Entities reference a Mesh ID.
* **Engine assets** - IDs prefixed with `[Engine]` (e.g., `[Engine] Base` shader, `[Engine] Hex` mesh). You should not
  try to delete these.

---

## The Hex Grid (Map)

Obliberry is built around a **pointy-top hex grid**.

* **Map Edit mode** - Paint tiles, set tile types (terrain, walls, etc.)
* **Tile types** - Each has an ID, texture, and color. Configured in the Tile Editor panel.
* **Map file** - Saved as `.obmap` (binary). Referenced by the scene.
* **Movement** - Entities with `MovementComponent` navigate using the grid's pathfinding.
* **DirectionalTexture** - Uses the 6 hex directions (0-5) for sprite rotation.

---

## ObSL Scripting

**ObSL (Obliberry Scripting Language)** is the built-in scripting language.

* Files: `.obsl` in `assets/scripts/`
* Attached via `ScriptComponent` (array of paths)
* Runs in **Play mode** only
* Can read/write any component, create/destroy entities, play audio, load scenes, etc.

See the [ObSL Getting Started](../scripting/getting-started.md) for syntax and API reference.

---

## Project Structure

```
my-project/
├── project.json          # Project config (name, start_scene)
├── assets/
│   ├── scenes/           # *.json scene files
│   ├── maps/             # *.obmap binary map files
│   ├── prefabs/          # *.json prefab files
│   ├── scripts/          # *.obsl script files
│   ├── textures/         # *.png, *.jpg etc
│   └── fonts/            # *.ttf / otf
```

> [!NOTE]
> Materials and meshes are **not** separate asset files. They're stored inline in scene/prefab JSON and created in the
> editor via the Inspector. The Project Browser shows them for reference, but they live in the scene/prefab data.

---

## Exporting: .obpak

When you're done, **File → Export Project** creates a single `.obpak` file containing:

* All scenes (as msgpack-encoded JSON)
* All assets (textures, shaders, fonts, maps, scripts)
* `project.json`

Exporting will also copy the runtime over to the export directory with the same name as the project.

The `obliberry_runtime` loads `.obpak` files directly, that's your "game" executable

---

## Summary

| Concept       | Key Point                                                  |
|---------------|------------------------------------------------------------|
| **Entity**    | an ID. Add components to give it behavior.                 |
| **Component** | data. One per type per entity.                             |
| **System**    | Logic that runs on entities with matching components.      |
| **Registry**  | The entity list (left panel). Shows hierarchy.             |
| **Asset**     | Referenced by ID. Materials link shaders + textures.       |
| **Map**       | Hex grid painted in Map Edit mode. Saved as `.obmap`.      |
| **Script**    | ObSL files attached via ScriptComponent. Run in Play mode. |
| **Prefab**    | Reusable entity template. Instantiate from script.         |
| **Export**    | Packages everything into `.obpak` for the runtime.         |
