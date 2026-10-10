# ObSL EngineLib - API Reference

The **EngineLib** is the bridge between ObSL scripts and the engine. It is registered at engine startup by
`Scripting::EngineLib` (`src/Scripting/EngineLib/`). This page is the authoritative list of script-visible functions,
organized by the ten registered modules.

## Conventions

- **Numbers** are doubles in ObSL; parameters are cast to `float`/`int` as needed by the engine.
- **Void** functions return `null`; failing lookups usually return `null` rather than throwing.
- **Booleans** sometimes accept a number (`0` = false, non-zero = true); this is noted per-function.
- **Composite values** come back as ObSL objects (`{x, y}`) or arrays (`[x, y, z]`), noted per-function.
- Most functions silently no-op (or return a zero value) when the relevant engine system isn't available e.g. `null`/
  `0`/
  `false` when there is no context, camera, input manager, or UI system.

## Thread safety

Scripts run in parallel on interpreter workers. The API handles synchronization:

- **Reads** use the shared registry mutex or per-module locks.
- **Most writes** are deferred through `ScriptCommandBuffer` / `UICommandBuffer` and applied on the main thread
  after the script pass. An immediate getter can still return the old value.
- **Entity creation and prefab instantiation** are synchronous so they can return an entity object.
- **Save data** is accessed synchronously under the save manager's mutex.

Most registry setters fall back to a direct write when there is no command buffer. `Transform.TryMoveTo`
requires an active script command buffer.

## Script hooks and globals

| Name               | Description                                                                                                              |
|--------------------|--------------------------------------------------------------------------------------------------------------------------|
| `this`             | The entity the script is attached to (an [entity object](#entity-objects)). Bound per script instance by `ScriptSystem`. |
| `fn on_update(dt)` | Optional. Called every frame with the raw (unscaled) delta time in seconds.                                              |
| `fn on_destroy()`  | Optional. Called when the entity is marked for destruction (`DestroyTagComponent`).                                      |
| `fn on_exit()`     | Optional. Called when the scene exits.                                                                                   |
| _(top-level code)_ | Runs once when the script is loaded.                                                                                     |

There is no `dt` global use `get_dt()` (time-scaled) or `GetRawDt()` (unscaled) outside `on_update`.

---

## Core & Window

| Function                           | Args               | Returns | Description                                             |
|------------------------------------|--------------------|---------|---------------------------------------------------------|
| `get_dt()`                         | -                  | number  | Time-scaled frame delta time (`deltaTime * timeScale`). |
| `Window_GetWidth()`                | -                  | number  | Current window width in pixels.                         |
| `Window_GetHeight()`               | -                  | number  | Current window height in pixels.                        |
| `Window_SetFullscreen(fullscreen)` | `bool` or `number` | void    | Enter/leave fullscreen (number: non-zero = true).       |
| `CloseWindow()`                    | -                  | void    | Requests the window to close.                           |

## Audio

| Function                    | Args                             | Returns | Description                                      |
|-----------------------------|----------------------------------|---------|--------------------------------------------------|
| `PlaySound2D(path, volume)` | `path: string`, `volume: number` | void    | Plays a one-shot 2D sound at `volume` (0.0-1.0). |
| `PlayMusic(path, volume)`   | `path: string`, `volume: number` | void    | Starts looping music playback.                   |
| `StopMusic()`               | -                                | void    | Stops the currently playing music.               |
| `SetMasterVolume(volume)`   | `volume: number`                 | void    | Sets the global master volume.                   |

## Camera

| Function                          | Args           | Returns            | Description                                                                                                                  |
|-----------------------------------|----------------|--------------------|------------------------------------------------------------------------------------------------------------------------------|
| `Camera_GetPosition()`            | -              | object `{x, y, z}` | The camera's world position.                                                                                                 |
| `Camera_SetPosition(x, y, z)`     | numbers        | bool               | Sets the camera position. `true` on success.                                                                                 |
| `Camera_Move(dx, dy, dz)`         | numbers        | bool               | Translates the camera by the given world-space delta.                                                                        |
| `Camera_PanScreenSpace(dx, dy)`   | numbers        | bool               | Pans by a screen-space delta, compensating for zoom and camera rotation (z ignored). Use this for player-controlled cameras. |
| `Camera_GetZoom()`                | -              | number             | Current zoom level.                                                                                                          |
| `Camera_SetZoom(zoom)`            | `zoom: number` | bool               | Sets the zoom level.                                                                                                         |
| `Camera_GetAngleX()`              | -              | number             | Camera tilt angle (degrees).                                                                                                 |
| `Camera_GetAngleZ()`              | -              | number             | Camera rotation angle (degrees).                                                                                             |
| `Camera_SetAngle(angleX, angleZ)` | numbers        | bool               | Sets both camera angles.                                                                                                     |

## Input

Key names are strings (e.g. `"Space"`, `"W"`, `"Esc"`) resolved via the input manager's key mappings. Mouse buttons are
numbers: `0` = left, `1` = right, `2` = middle.

| Function                        | Args   | Returns         | Description                                                                                 |
|---------------------------------|--------|-----------------|---------------------------------------------------------------------------------------------|
| `Input_IsKeyDown(keyName)`      | string | bool            | `true` while the key is held down.                                                          |
| `Input_IsKeyPressed(keyName)`   | string | bool            | `true` only on the frame the key is first pressed (edge triggered).                         |
| `Input_IsKeyReleased(keyName)`  | string | bool            | `true` only on the frame the key is released.                                               |
| `Input_IsMouseDown(button)`     | number | bool            | `true` while the mouse button is held.                                                      |
| `Input_IsMousePressed(button)`  | number | bool            | `true` on the frame the mouse button is pressed.                                            |
| `Input_IsMouseReleased(button)` | number | bool            | `true` on the frame the mouse button is released.                                           |
| `Input_GetMouseX()`             | -      | number          | Mouse X in pixels (viewport-adjusted).                                                      |
| `Input_GetMouseY()`             | -      | number          | Mouse Y in pixels (viewport-adjusted).                                                      |
| `Input_GetScrollX()`            | -      | number          | Horizontal scroll delta since last frame.                                                   |
| `Input_GetScrollY()`            | -      | number          | Vertical scroll delta since last frame.                                                     |
| `Input_GetMouseWorldPos()`      | -      | object `{x, y}` | World-space position under the cursor (uses the editor viewport framebuffer in the editor). |

## Hex map

| Function                            | Args                    | Returns                       | Description                                                                                                                                                                                                               |
|-------------------------------------|-------------------------|-------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `Math_WorldToHex(x, y)`             | numbers                 | object `{q, r}`               | Converts a world position to hex coordinates.                                                                                                                                                                             |
| `GetSelectedHex()`                  | -                       | object `{hasSelection, q, r}` | The currently selected hex (if any).                                                                                                                                                                                      |
| `SetSelectedHex(q, r)`              | numbers                 | void                          | Selects the hex if it exists and is walkable; otherwise clears the selection.                                                                                                                                             |
| `SetPathToHex(entityId, q, r)`      | numbers                 | bool                          | Requests an A\* path to `(q, r)`. Requires Movement, Transform, and a map when executed. `true` means numeric arguments were accepted; it does not confirm a valid entity or path. Invalid argument types return `false`. |
| `ClearSelectionOverlay()`           | -                       | void                          | Clears the selection highlight on the map.                                                                                                                                                                                |
| `ClearPathTarget()`                 | -                       | void                          | Clears the path overlay.                                                                                                                                                                                                  |
| `Map_IsHexWalkable(q, r)`           | numbers                 | bool                          | `true` if the hex exists and is walkable.                                                                                                                                                                                 |
| `Map_SetHexWalkable(obj, walkable)` | object `{q, r}`, bool   | void                          | Sets the walkable state of the hex at `obj.q`, `obj.r`.                                                                                                                                                                   |
| `Map_SetTileType(obj, type)`        | object `{q, r}`, number | void                          | Sets the tile type (0-255) of the hex at `obj.q`, `obj.r`.                                                                                                                                                                |
| `Map_GetMapEntity()`                | -                       | entity or `null`              | The entity holding the scene's `MapComponent`.                                                                                                                                                                            |
| `Hex_Distance(aQ, aR, bQ, bR)`      | numbers                 | number                        | Hex-grid distance between two hexes.                                                                                                                                                                                      |
| `Hex_GetNeighbors(q, r)`            | numbers                 | array of `{q, r}`             | The six neighbors of the hex.                                                                                                                                                                                             |
| `Hex_HexToWorld(q, r)`              | numbers                 | object `{x, y}`               | Converts hex coordinates to a world position.                                                                                                                                                                             |

Queued path requests are evaluated at command-buffer flush. Missing components/maps or an empty path can leave the
entity unmoved even when `SetPathToHex` returned `true`. Hex movement also checks walkability and solids at each
attempted destination.

Hex coordinates are **odd-r offset** (pointy-top hexes). See `src/Math/HexMath.h` for the underlying math.

## Registry global functions

| Function                  | Args              | Returns          | Description                                                                                  |
|---------------------------|-------------------|------------------|----------------------------------------------------------------------------------------------|
| `GetEntity(id)`           | number            | entity or `null` | Wraps an entity by numeric id.                                                               |
| `Find(name)`              | string            | entity or `null` | Finds the first entity with the given name.                                                  |
| `CreateEntity(name)`      | string (optional) | entity           | Creates an empty entity synchronously (default name `"NewEntity"`); add required components. |
| `Instantiate(prefabPath)` | string            | entity or `null` | Instantiates a prefab (`assets/prefabs/*.json`). `null` on failure.                          |
| `DestroyEntity(id)`       | number            | void             | Destroys the entity (deferred).                                                              |

`Instantiate` acquires the prefab's catalog assets and their dependencies before creating the entity, and retains
them in the current scene. They do not need to be loaded beforehand; missing required assets cause `null` to be
returned.

`DestroyEntity` and `entity.Destroy()` also remove children. To run `on_destroy` before deletion, use
`entity.AddComponent("DestroyTag")` instead.

## Entity objects

Entity objects are returned by `GetEntity`, `Find`, `CreateEntity`, `Instantiate`, `Map_GetMapEntity`, `GetChildren`,
`GetParent`, and `this`.

**Data fields:** `id` (numeric runtime handle), `name` (string snapshot). Use `GetName()` for the current name.
Reacquire entity objects after a scene change; runtime handles belong to the current registry.

| Method                            | Args             | Returns             | Description                                                                                              |
|-----------------------------------|------------------|---------------------|----------------------------------------------------------------------------------------------------------|
| `SetName(name)`                   | string           | void                | Renames the entity.                                                                                      |
| `GetName()`                       | -                | string              | The entity's name.                                                                                       |
| `GetComponent(name)`              | string           | component or `null` | Wraps a built-in component (names below).                                                                |
| `HasComponent(name)`              | string           | bool                | Whether the entity has the built-in component.                                                           |
| `AddComponent(name)`              | string           | void                | Adds the built-in component if not present.                                                              |
| `RemoveComponent(name)`           | string           | void                | Removes the built-in component.                                                                          |
| `GetComponents()`                 | -                | array of strings    | Names of the built-in components present.                                                                |
| `Destroy()`                       | -                | void                | Destroys the entity (deferred).                                                                          |
| `AddCustomComponent(name, value)` | string, any      | bool                | Stores GC-rooted script data for the current entity lifetime; not serialized or migrated between scenes. |
| `GetCustomComponent(name)`        | string           | any or `null`       | Reads back custom component data.                                                                        |
| `GetChildren()`                   | -                | array of entities   | Direct children (hierarchy).                                                                             |
| `GetParent()`                     | -                | entity or `null`    | The parent entity, if any.                                                                               |
| `SetParent(parent)`               | number or entity | void                | Reparents under the given entity (accepts an id or an entity object).                                    |
| `GetChildCount()`                 | -                | number              | Number of direct children.                                                                               |
| `Find(childName)`                 | string           | entity or `null`    | Finds a direct child by name.                                                                            |
| `SetPersistent(persistent)`       | bool             | void                | Marks the entity to survive scene loads.                                                                 |
| `IsPersistent()`                  | -                | bool                | Whether the entity is marked as persistent.                                                              |

**Built-in component names** (accepted by
`GetComponent`/`HasComponent`/`AddComponent`/`RemoveComponent`):
`"Transform"`, `"PointLight"`, `"Movement"`, `"MapState"`,
`"DirectionalTexture"`, `"BillboardTag"`, `"DestroyTag"`,
`"ParticleEmitter"`, `"Collider"`, `"SpriteSheet"`, `"SpriteAnimator"`.

Adding `"SpriteAnimator"` also adds `"SpriteSheet"` if it is missing.
Removing `"SpriteAnimator"` leaves the sprite component and its last
resolved pose.

### Persistence

Entities marked as **persistent** survive scene transitions with their child entities. Scripts are reinitialized.

```obsl
// Runs once when the script is loaded.
this.SetPersistent(true);
```

Deduplication uses the entity's **UUID**. A destination entity with the same UUID is replaced by the carried-over
entity. Entities with the same name and different UUIDs are kept separately. UUIDs are saved with scene entities;
save older scenes after loading them to retain their generated IDs.

Runtime entity IDs change during migration, so reacquire entity references after a scene change. All migrated
children are also marked persistent. `PersistentTagComponent` itself is runtime-only and is not saved in scene files.

> [!NOTE]
> Script variables and CustomData are not carried over. Use `save_set` / `save_get` for values needed across scenes
> and restore them when the script initializes. In-memory save values do not require disk saves to be enabled.
> Migration captures component data before `on_exit` runs.

### Component wrappers

These are the objects returned by `entity.GetComponent(name)`.

**Transform** : position/rotation/scale are `[x, y, z]` arrays.

| Method                 | Args    | Returns           | Description                                                                                          |
|------------------------|---------|-------------------|------------------------------------------------------------------------------------------------------|
| `SetPosition(x, y, z)` | numbers | void              | Teleports to local xyz, bypassing collision; world xyz for a root entity.                            |
| `SetRotation(x, y, z)` | numbers | void              | Sets local Euler rotation in radians.                                                                |
| `SetScale(x, y, z)`    | numbers | void              | Sets scale.                                                                                          |
| `TryMoveTo(x, y, z)`   | numbers | void              | Queues a collision-aware world destination for a root entity. Returns `null`, not a success boolean. |
| `GetPosition()`        | -       | array `[x, y, z]` | Current local position.                                                                              |
| `GetRotation()`        | -       | array `[x, y, z]` | Current local Euler rotation in radians.                                                             |
| `GetScale()`           | -       | array `[x, y, z]` | Current scale.                                                                                       |
| `IsMoving()`           | -       | bool              | `true` while the entity's movement component reports moving.                                         |

`TryMoveTo` accepts three finite numbers within the float range. It checks the destination when queued commands
are applied; a blocked destination leaves the position unchanged. The call returns `null`, so read the position
after the script pass to observe the result.

The entity must have a Transform and no parent. No Movement component or hex map is required. Only the entity's own
collider is checked at the destination; there is no swept test or sliding. Missing, trigger, or invalid colliders
bypass solid blocking. Invalid arguments are ignored.

**PointLight**

| Method                    | Args    | Returns | Description           |
|---------------------------|---------|---------|-----------------------|
| `SetColor(r, g, b)`       | numbers | void    | Sets light color.     |
| `SetIntensity(intensity)` | number  | void    | Sets light intensity. |
| `SetRadius(radius)`       | number  | void    | Sets light radius.    |

**Movement**

| Method                    | Args   | Returns | Description                                |
|---------------------------|--------|---------|--------------------------------------------|
| `GetIsMoving()`           | -      | bool    | `true` while the entity is walking a path. |
| `SetIsMoving(moving)`     | bool   | void    | Overrides the moving state.                |
| `SetTimePerStep(seconds)` | number | void    | Time between path steps.                   |

**MapState**

| Method              | Args | Returns        | Description                              |
|---------------------|------|----------------|------------------------------------------|
| `GetHasSelection()` | -    | bool           | Whether the map has an active selection. |
| `GetSelectedHex()`  | -    | array `[q, r]` | The selected hex.                        |
| `GetPathToHex()`    | -    | array `[q, r]` | The current path target hex.             |

**DirectionalTexture**

| Method            | Args   | Returns | Description                                    |
|-------------------|--------|---------|------------------------------------------------|
| `SetIndex(index)` | number | void    | Sets the active direction texture index (0-5). |

**BillboardTag / DestroyTag** empty wrapper objects (presence/absence is the state/tag; there are no methods).

**ParticleEmitter**

| Method              | Args           | Returns | Description                                                                                                                |
|---------------------|----------------|---------|----------------------------------------------------------------------------------------------------------------------------|
| `SetEmitRate(rate)` | number         | void    | Sets the particle emission rate.                                                                                           |
| `SetActive(active)` | bool or number | void    | Enables/disables emission.                                                                                                 |
| `GetActive()`       | -              | bool    | Whether emission is active.                                                                                                |
| `GetAliveCount()`   | -              | number  | Live particle count from the latest completed ParticleSystem update; `0` before initialization or for a missing component. |

**Collider**

| Method                | Args   | Returns | Description                                                   |
|-----------------------|--------|---------|---------------------------------------------------------------|
| `GetIsTrigger()`      | —      | bool    | Whether overlaps use trigger hooks.                           |
| `SetIsTrigger(value)` | bool   | void    | Changes the trigger flag.                                     |
| `GetLayer()`          | —      | number  | Layer index, 0–31.                                            |
| `SetLayer(index)`     | number | void    | Sets a finite integer index in 0–31; ignores invalid input.   |
| `GetMask()`           | —      | number  | Unsigned 32-bit mask as an ObSL number.                       |
| `SetMask(mask)`       | number | void    | Sets a finite integer in 0–4294967295; ignores invalid input. |

Configure shape, orientation, and dimensions in the editor or scene JSON. Default layer is `0`, and the default mask is
`4294967295` (all layers). **Both** masks must accept the other's layer for overlap events or movement blocking.
Mask `0` disables new overlaps and blocking; an existing pair can still emit an exit event.

```obsl
var collider = this.GetComponent("Collider");
if (collider != null) {
    collider.SetLayer(2);
    collider.SetMask(8); // Accept layer 3 (bit 3). The other collider must accept layer 2 too.
}
```

Solid colliders block the collision-aware movement helper and hex movement. Direct `SetPosition` calls teleport.
There is no rigid-body solver or automatic push-apart response. If either collider is a trigger, the pair uses
trigger hooks. `other` is an entity wrapper, or `null` when that entity is no longer valid.

```obsl
fn on_collision_enter(other) { /* first overlap */ }
fn on_collision_stay(other) { /* continuing overlap */ }
fn on_collision_exit(other) { /* overlap ended */ }

fn on_trigger_enter(other) { /* first trigger overlap */ }
fn on_trigger_stay(other) { /* continuing trigger overlap */ }
fn on_trigger_exit(other) { /* trigger overlap ended */ }
```

**SpriteSheet**

Controls the sheet and displayed frame of a static sprite.

| Method                   | Args           | Returns | Description                                                                   |
|--------------------------|----------------|---------|-------------------------------------------------------------------------------|
| `GetFrame()`             | —              | number  | Current zero-based sheet frame index.                                         |
| `GetColumns()`           | —              | number  | Number of sheet columns, or zero if no sheet is assigned.                     |
| `GetRows()`              | —              | number  | Number of sheet rows, or zero if no sheet is assigned.                        |
| `GetTexture()`           | —              | string  | Registered texture resource ID.                                               |
| `SetFrame(index)`        | number         | void    | Selects a frame inside the current grid.                                      |
| `SetTexture(id)`         | string         | void    | Acquires and assigns a texture from `assets.json`; an empty string clears it. |
| `SetGrid(columns, rows)` | number, number | void    | Sets the grid dimensions.                                                     |

For animated entities, the animation system writes the sheet and frame.
Manual SpriteSheet changes can therefore be overwritten by the animator.

**SpriteAnimator**

Controls playback of named clips from the assigned animation set.

| Method          | Args   | Returns | Description                                                                                    |
|-----------------|--------|---------|------------------------------------------------------------------------------------------------|
| `Play(name)`    | string | void    | Starts a valid clip. Selecting the current clip does not restart it or resume paused playback. |
| `Restart(name)` | string | void    | Starts a valid clip from its first frame, even if already selected.                            |
| `Pause()`       | —      | void    | Pauses playback at its current position.                                                       |
| `Resume()`      | —      | void    | Resumes the current valid clip.                                                                |
| `Stop()`        | —      | void    | Pauses and resets playback to the current clip's first frame.                                  |
| `IsPlaying()`   | —      | bool    | Whether playback is running.                                                                   |
| `GetClip()`     | —      | string  | Current clip name, or an empty string when none is selected.                                   |

Assign the animation set through the editor or scene data before playing
a clip. These bindings do not currently provide an animation-set setter.

When called from a script worker, mutations use the script command buffer.
An immediate read after a queued mutation may still return the previous state.

Looping clips repeat. Non-looping clips stop on their final frame.

## Scene management

| Function                | Args   | Returns | Description                                                                                                                                                                    |
|-------------------------|--------|---------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `LoadScene(scenePath)`  | string | void    | Requests a scene load (deferred; performed by the engine). Persistent entities in the current scene are carried over to the new scene (see [Scene Persistence](#persistence)). |
| `GetCurrentScenePath()` | -      | string  | VFS path of the current scene (e.g. `"assets/scenes/level1.json"`).                                                                                                            |

## Time

| Function              | Args             | Returns        | Description                                                                                                                                                                       |
|-----------------------|------------------|----------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `GetFrameCount()`     | -                | number         | Frames elapsed since the engine started.                                                                                                                                          |
| `GetTimeScale()`      | -                | number         | Current time scale (default `1.0`).                                                                                                                                               |
| `SetTimeScale(scale)` | number           | bool           | Sets the time scale, clamped to ≥ 0. `true` on success.                                                                                                                           |
| `GetRawDt()`          | -                | number         | Raw (unscaled) delta time; see also `get_dt()` (scaled).                                                                                                                          |
| `SetTimeout(fn, ms)`  | callable, number | bool or `null` | Calls `fn()` after `ms` milliseconds. Runs on the main thread with a command buffer so registry mutations work. Returns `true` when scheduled, `null` for invalid argument types. |

## Save data

The save module provides an explicit project-wide key/value store.

Enable saves in **File → Project Settings** before using file operations. When persistent saves are disabled or the
manager could not be configured, file operations fail safely; `save_list()` returns an empty array.

| Function                   | Returns                | Description                                                                          |
|----------------------------|------------------------|--------------------------------------------------------------------------------------|
| `save_set(key, value)`     | bool                   | Stores a boolean, finite number, or string in the active in-memory data.             |
| `save_get(key)`            | value or `null`        | Returns the stored value, or `null` when the key is absent.                          |
| `save_has(key)`            | bool                   | Returns whether the active data contains the key.                                    |
| `save_remove(key)`         | bool                   | Removes the key and returns whether it existed.                                      |
| `save_clear()`             | `null`                 | Removes every stored value but keeps the active filename and metadata.               |
| `save_new()`               | `null`                 | Resets the in-memory save and clears the active filename. It does not create a file. |
| `save_create(displayName)` | filename string/`null` | Creates a new file from the current data, makes it active, and returns its filename. |
| `save_write()`             | bool                   | Writes the active data to its current file. Returns `false` when no file is active.  |
| `save_load(filename)`      | bool                   | Loads a save filename and makes it active.                                           |
| `save_delete(filename)`    | bool                   | Deletes a save filename. Deleting the active file clears its active filename.        |
| `save_list()`              | array                  | Returns valid saves sorted by `updated_at`, newest first.                            |

Each object returned by `save_list()` contains:

| Field          | Type   | Description                                               |
|----------------|--------|-----------------------------------------------------------|
| `filename`     | string | Filename to pass to `save_load` or `save_delete`.         |
| `display_name` | string | User-facing name supplied when the save was created.      |
| `created_at`   | number | Creation time in UTC milliseconds since the Unix epoch.   |
| `updated_at`   | number | Last write time in UTC milliseconds since the Unix epoch. |

Use non-empty string keys. `save_set("", value)` returns `false` and stores nothing.

The store is explicit project-wide memory: it remains available across scene transitions. Enabling saves configures
disk file operations; it is not required for in-memory `save_set` / `save_get`.

Use the array index only to let the player select an entry. Pass that entry's `filename` to file operations because
the ordering can change after a save is written.

```obsl
save_new();
save_set("level", 1);
save_set("questsomething.complete", true);

var filename = save_create("First Save");

// Later after changing the in memory data:
save_set("level", 2);
save_write();

// You can also select an entry from this array for example a menu or so:
var saves = save_list();
if (len(saves) > 0) {
    save_load(saves[0].filename);
}
```

See [Save-game format](../formats/save-json.md) for the on-disk layout and storage directories.

## UI (GUI)

The UI module is still **WIP** . Mutations are deferred to the main thread; element lookups return wrapper objects keyed
by element name. Colors are `[r, g, b, a]` arrays, positions/sizes are `[x, y]` arrays.

### Global functions

| Function               | Args              | Returns           | Description                                      |
|------------------------|-------------------|-------------------|--------------------------------------------------|
| `FindUI(name)`         | string            | element or `null` | Looks up an existing element by name.            |
| `CreateUIButton(name)` | string (optional) | button            | Creates a button (default name `"NewButton"`).   |
| `CreateUIText(name)`   | string (optional) | text              | Creates a text element (default `"NewText"`).    |
| `CreateUIRect(name)`   | string (optional) | rect              | Creates a rect element (default `"NewRect"`).    |
| `CreateUIImage(name)`  | string (optional) | image             | Creates an image element (default `"NewImage"`). |
| `DestroyUI(name)`      | string            | void              | Removes the element from its parent.             |

### Base methods (all element types)

| Method                | Args           | Returns        | Description              |
|-----------------------|----------------|----------------|--------------------------|
| `GetName()`           | -              | string         | Element name.            |
| `GetPosition()`       | -              | array `[x, y]` | UI-space position.       |
| `GetSize()`           | -              | array `[x, y]` | UI-space size.           |
| `IsVisible()`         | -              | bool           | `VISIBLE` flag.          |
| `IsEnabled()`         | -              | bool           | `ENABLED` flag.          |
| `IsFocused()`         | -              | bool           | `FOCUSED` flag.          |
| `SetPosition(x, y)`   | numbers        | void           | Sets position.           |
| `SetSize(x, y)`       | numbers        | void           | Sets size.               |
| `SetVisible(visible)` | bool or number | void           | Sets the `VISIBLE` flag. |

### Button methods

| Method                           | Args    | Returns              | Description                                                 |
|----------------------------------|---------|----------------------|-------------------------------------------------------------|
| `GetText()`                      | -       | string               | Button label.                                               |
| `GetTextColor()`                 | -       | array `[r, g, b, a]` | Text color.                                                 |
| `GetBackgroundColor()`           | -       | array `[r, g, b, a]` | Background color.                                           |
| `WasClicked()`                   | -       | bool                 | `true` if clicked this frame.                               |
| `IsHovered()`                    | -       | bool                 | `true` while hovered.                                       |
| `IsHeld()`                       | -       | bool                 | `true` while held down.                                     |
| `SetText(text)`                  | string  | void                 | Sets the label.                                             |
| `SetTextColor(r, g, b, a)`       | numbers | void                 | Sets the text color.                                        |
| `SetBackgroundColor(r, g, b, a)` | numbers | void                 | Sets the background color.                                  |
| `SetFont(fontName)`              | string  | void                 | Acquires and sets a font from `assets.json` by resource ID. |
| `GetFont()`                      | -       | string               | The font resource key.                                      |

### Text methods

| Method                 | Args    | Returns              | Description                                                 |
|------------------------|---------|----------------------|-------------------------------------------------------------|
| `GetText()`            | -       | string               | Text content.                                               |
| `GetColor()`           | -       | array `[r, g, b, a]` | Text color.                                                 |
| `SetText(text)`        | string  | void                 | Sets content.                                               |
| `SetColor(r, g, b, a)` | numbers | void                 | Sets color.                                                 |
| `SetFont(fontName)`    | string  | void                 | Acquires and sets a font from `assets.json` by resource ID. |

### Rect and Image methods

| Method                 | Args    | Returns              | Description         |
|------------------------|---------|----------------------|---------------------|
| `GetColor()`           | -       | array `[r, g, b, a]` | Element color.      |
| `SetColor(r, g, b, a)` | numbers | void                 | Sets element color. |

## See also

- [Getting started with ObSL](getting-started.md) lifecycle, hooks, and full examples.
- The language itself: [ObSL README](https://github.com/torkelicious/ObSL) and `external/obsl/docs/`.
- Source for this page: `src/Scripting/EngineLib/` (the `EngineLib*` module files).
