# Scenes

A **scene** is a self-contained level or screen in your project. It holds everything that exists together at one time:
the map, entities, UI, lighting, music, and references to project assets.

---

## What a Scene Contains

| Section              | Description                                                                  |
|----------------------|------------------------------------------------------------------------------|
| **Scene Properties** | Name, background clear color, music, ambient light, lighting enablement      |
| **Registry**         | All entities and their components (Transform, Mesh, Material, Script, etc.)  |
| **Map**              | hex-grid map (stored as a `.obmap` file, referenced by the scene)            |
| **UI**               | UI element tree (text, buttons, images) !! separate from the entity Registry |
| **Post-Processing**  | Fullscreen effect chain (bloom, CRT, custom shaders)                         |

Asset definitions are project-wide and live in [`assets.json`](../formats/assets-json.md). A scene stores only the IDs
it uses.

> [!NOTE]
> Scenes are saved as JSON files under `assets/scenes/` in your project. The format is documented
> in [scene-json.md](../formats/scene-json.md) if you need the technical details.

---

## Creating & Managing Scenes

### New Scene

**Scene → Create Scene** creates a blank scene file. Use **Scene → Switch To** to load it.

### Switch Scenes

**Scene → Switch To** lists scenes in `assets/scenes/`. If the current scene or map is dirty, the editor prompts to save
or discard changes before switching.

### Scene Properties

![Scene Properties](img/scene-properties-window.png)

**Scene → Edit Scene Properties** - Edit:

- **Name** - Display name (shown in the scene list)
- **Clear Color** - Background color (RGBA)
- **Background Music** - Path to music file (relative to project assets)
- **Ambient Light** - Global ambient intensity (0-1), also feeds into the map lightmap
- **Enable Lighting System** - Toggles scene lighting (saved as `properties.lighting`).

---

## The MAP Entity

If your scene has a hex-grid map, there's a special invisible entity named **"MAP"** in the Registry. It holds:

- `MapComponent` - The loaded `.obmap` data
- `MapStateComponent` - Runtime state (selection, path highlights)

You don't edit these components directly. Use **Map Edit mode** to paint the map, and the MAP entity updates
automatically.

> [!IMPORTANT]
> The map file path is stored _in the scene_, not in the MAP entity. When you save the scene, the map reference is saved
> with it.

---

## Scene Workflow

| Task                          | How                                                                                            |
|-------------------------------|------------------------------------------------------------------------------------------------|
| **Start a new level**         | Scene → Create Scene, then Scene → Switch To, then Map Edit mode to paint the grid             |
| **Reuse a map across scenes** | In Map Edit mode: Map → Save Map As... → give it a name. Then in the new scene: Map → Load Map |
| **Duplicate a scene**         | Copy the `.json` file in `assets/scenes/`, rename it, then Scene → Switch To                   |
| **Set the starting scene**    | Edit `project.json` → `start_scene` field (relative to `assets/scenes/`)                       |

---

## Saving

- **Ctrl+S** - Save current scene (or map in Map Edit mode)
- The editor prompts for dirty scene changes when switching scenes, entering Play mode, or quitting.
- Save the scene, map, and asset drafts before exporting; export reads disk files and does not prompt to save.

Copying a scene JSON also copies its entity UUIDs. Preserve those UUIDs only when the entities should represent the
same logical identities across scenes. For independent entities, remove their `uuid` fields before loading, then
save to record the newly generated identities. Hierarchy still uses array indices in `parent`.

---

## Scene Loading Order

The engine loads in this order:

1. **Properties** - Reads clear color, music, and ambient light.
2. **Referenced assets** - Scans the scene's IDs, resolves material and animation dependencies through `assets.json`,
   and loads only the required subset.
3. **Grid** - Loads the `.obmap` file and creates the MAP entity.
4. **Post-Processing** - Restores the effect chain (or uses the default chain when absent).
5. **Entities** - Creates entities, adds components, then reparents children.
6. **UI** - Builds the UI element tree.

The scene retains the acquired assets for its lifetime. Switching scenes releases that scope and unloads project assets
that are no longer referenced. Persistent entities reacquire their required assets for the destination scene.

---

## Scene Persistence

Scripts can mark an entity with `SetPersistent(true)`. Scene transitions migrate it and its child subtree, preserve
UUIDs, and replace destination entities with matching UUIDs. Names do not control deduplication. Runtime IDs change,
and all migrated members are marked persistent in the new registry.

Scripts are initialized again. CustomData and script variables are not included in the serialized migration.
Use explicit `save_set` / `save_get` values and restore the relevant state in the new script instance. See
[Persistence](../scripting/api-reference.md#persistence) for details.

---

## See Also

- [Post-Processing](post-processing.md) - Editing the scene's fullscreen effect chain
- [Asset Catalog](../formats/assets-json.md) - Project-wide asset definitions and lazy loading
- [Component Reference](components.md) - All components you can add to entities
- [Map Editing](usage.md#map-edit-mode) - Painting and configuring the hex grid
- [Prefabs](prefabs.md) - Creating reusable entity templates
- [Project Structure](concepts.md#project-structure) - Where scenes live on disk
