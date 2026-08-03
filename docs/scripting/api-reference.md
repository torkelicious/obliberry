# ObSL EngineLib - API Reference

The **EngineLib** is the bridge between ObSL scripts and the engine. It is registered at engine startup by
`Scripting::EngineLib` (`src/Scripting/EngineLib/`). This page is the authoritative list of script-visible functions,
organized by the nine registered modules.

## Conventions

* **Numbers** are doubles in ObSL; parameters are cast to `float`/`int` as needed by the engine.
* **Void** functions return `nil` (`null`); failing lookups usually return `nil` rather than throwing.
* **Booleans** sometimes accept a number (`0` = false, non-zero = true); this is noted per-function.
* **Composite values** come back as ObSL objects (`{x, y}`) or arrays (`[x, y, z]`), noted per-function.
  > I apologize if this is confusing, Object were implemented before arrays and some things may have gotten a bit
  fragmented.
* Most functions silently no-op (or return a zero value) when the relevant engine system isn't available e.g. `nil`/`0`/
  `false` when there is no context, camera, input manager, or UI system.

## Thread safety

Scripts run in parallel on interpreter workers. The API hides the synchronization:

* **Reads** are protected by per-module mutexes (camera, audio, window, scene management, time, input-camera) or the
  shared registry mutex (`g_RegistryMutex`).
* **Mutations** (registry and UI writes) are *deferred* through `ScriptCommandBuffer` / `UICommandBuffer` and applied on
  the main thread after the parallel script pass. If no command buffer is present (e.g. scripts run outside the normal
  frame path), the EngineLib falls back to direct mutation under the registry mutex.

You can call any function from any hook without extra bookkeeping.

## Script hooks and globals

| Name               | Description                                                                                                              |
|--------------------|--------------------------------------------------------------------------------------------------------------------------|
| `this`             | The entity the script is attached to (an [entity object](#entity-objects)). Bound per script instance by `ScriptSystem`. |
| `fn on_update(dt)` | Optional. Called every frame with the raw (unscaled) delta time in seconds.                                              |
| `fn on_destroy()`  | Optional. Called when the entity is marked for destruction (`DestroyTagComponent`).                                      |
| `fn on_exit()`     | Optional. Called when the scene exits.                                                                                   |
| *(top-level code)* | Runs once when the script is loaded.                                                                                     |

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

| Function                       | Args    | Returns                       | Description                                                                                                                                                    |
|--------------------------------|---------|-------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `Math_WorldToHex(x, y)`        | numbers | object `{q, r}`               | Converts a world position to hex coordinates.                                                                                                                  |
| `GetSelectedHex()`             | -       | object `{hasSelection, q, r}` | The currently selected hex (if any).                                                                                                                           |
| `SetSelectedHex(q, r)`         | numbers | void                          | Selects the hex if it exists and is walkable; otherwise clears the selection.                                                                                  |
| `SetPathToHex(entityId, q, r)` | numbers | bool                          | Finds an A* path for the entity to `(q, r)` and starts movement (requires `MovementComponent` + `TransformComponent`). `false` if the entity/args are invalid. |
| `ClearSelectionOverlay()`      | -       | void                          | Clears the selection highlight on the map.                                                                                                                     |
| `ClearPathTarget()`            | -       | void                          | Clears the path overlay.                                                                                                                                       |
| `Map_IsHexWalkable(q, r)`      | numbers | bool                          | `true` if the hex exists and is walkable.                                                                                                                      |
| `Map_GetMapEntity()`           | -       | entity or `nil`               | The entity holding the scene's `MapComponent`.                                                                                                                 |
| `Hex_Distance(aQ, aR, bQ, bR)` | numbers | number                        | Hex-grid distance between two hexes.                                                                                                                           |
| `Hex_GetNeighbors(q, r)`       | numbers | array of `{q, r}`             | The six neighbors of the hex.                                                                                                                                  |
| `Hex_HexToWorld(q, r)`         | numbers | object `{x, y}`               | Converts hex coordinates to a world position.                                                                                                                  |

Hex coordinates are **odd-r offset** (pointy-top hexes). See `src/Math/HexMath.h` for the underlying math.

## Registry global functions

| Function                  | Args              | Returns         | Description                                                        |
|---------------------------|-------------------|-----------------|--------------------------------------------------------------------|
| `GetEntity(id)`           | number            | entity or `nil` | Wraps an entity by numeric id.                                     |
| `Find(name)`              | string            | entity or `nil` | Finds the first entity with the given name.                        |
| `CreateEntity(name)`      | string (optional) | entity          | Creates a new entity (default name `"NewEntity"`).                 |
| `Instantiate(prefabPath)` | string            | entity or `nil` | Instantiates a prefab (`assets/prefabs/*.json`). `nil` on failure. |
| `DestroyEntity(id)`       | number            | void            | Destroys the entity (deferred).                                    |

## Entity objects

Entity objects are returned by `GetEntity`, `Find`, `CreateEntity`, `Instantiate`, `Map_GetMapEntity`, `GetChildren`,
`GetParent`, and `this`.

**Data fields:** `id` (number), `name` (string).

| Method                            | Args             | Returns            | Description                                                                         |
|-----------------------------------|------------------|--------------------|-------------------------------------------------------------------------------------|
| `SetName(name)`                   | string           | void               | Renames the entity.                                                                 |
| `GetName()`                       | -                | string             | The entity's name.                                                                  |
| `GetComponent(name)`              | string           | component or `nil` | Wraps a built-in component (names below).                                           |
| `HasComponent(name)`              | string           | bool               | Whether the entity has the built-in component.                                      |
| `AddComponent(name)`              | string           | void               | Adds the built-in component if not present.                                         |
| `RemoveComponent(name)`           | string           | void               | Removes the built-in component.                                                     |
| `GetComponents()`                 | -                | array of strings   | Names of the built-in components present.                                           |
| `Destroy()`                       | -                | void               | Destroys the entity (deferred).                                                     |
| `AddCustomComponent(name, value)` | string, any      | bool               | Stores arbitrary script data on the entity under `name` (persists and survives GC). |
| `GetCustomComponent(name)`        | string           | any or `nil`       | Reads back custom component data.                                                   |
| `GetChildren()`                   | -                | array of entities  | Direct children (hierarchy).                                                        |
| `GetParent()`                     | -                | entity or `nil`    | The parent entity, if any.                                                          |
| `SetParent(parent)`               | number or entity | void               | Reparents under the given entity (accepts an id or an entity object).               |
| `GetChildCount()`                 | -                | number             | Number of direct children.                                                          |
| `Find(childName)`                 | string           | entity or `nil`    | Finds a direct child by name.                                                       |

**Built-in component names** (accepted by `GetComponent`/`HasComponent`/`AddComponent`/`RemoveComponent`):
`"Transform"`, `"PointLight"`, `"Movement"`, `"MapState"`, `"DirectionalTexture"`, `"BillboardTag"`, `"DestroyTag"`,
`"ParticleEmitter"`.

### Component wrappers

These are the objects returned by `entity.GetComponent(name)`.

**Transform** : position/rotation/scale are `[x, y, z]` arrays.

| Method                 | Args    | Returns           | Description                                                  |
|------------------------|---------|-------------------|--------------------------------------------------------------|
| `SetPosition(x, y, z)` | numbers | void              | Sets world/local position.                                   |
| `SetRotation(x, y, z)` | numbers | void              | Sets Euler rotation.                                         |
| `SetScale(x, y, z)`    | numbers | void              | Sets scale.                                                  |
| `GetPosition()`        | -       | array `[x, y, z]` | Current position.                                            |
| `GetRotation()`        | -       | array `[x, y, z]` | Current rotation.                                            |
| `GetScale()`           | -       | array `[x, y, z]` | Current scale.                                               |
| `IsMoving()`           | -       | bool              | `true` while the entity's movement component reports moving. |

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

| Method              | Args           | Returns | Description                                             |
|---------------------|----------------|---------|---------------------------------------------------------|
| `SetEmitRate(rate)` | number         | void    | Sets the particle emission rate.                        |
| `SetActive(active)` | bool or number | void    | Enables/disables emission.                              |
| `GetActive()`       | -              | bool    | Whether emission is active.                             |
| `GetAliveCount()`   | -              | number  | Currently returns `0` (runtime-only; not wired up yet). |

## Scene management

| Function                | Args   | Returns | Description                                                         |
|-------------------------|--------|---------|---------------------------------------------------------------------|
| `LoadScene(scenePath)`  | string | void    | Requests a scene load (deferred; performed by the engine).          |
| `GetCurrentScenePath()` | -      | string  | VFS path of the current scene (e.g. `"assets/scenes/level1.json"`). |

## Time

| Function              | Args   | Returns | Description                                             |
|-----------------------|--------|---------|---------------------------------------------------------|
| `GetFrameCount()`     | -      | number  | Frames elapsed since the engine started.                |
| `GetTimeScale()`      | -      | number  | Current time scale (default `1.0`).                     |
| `SetTimeScale(scale)` | number | bool    | Sets the time scale, clamped to ≥ 0. `true` on success. |
| `GetRawDt()`          | -      | number  | Raw (unscaled) delta time see also `get_dt()` (scaled). |

## UI (GUI)

The UI module is still **WIP** . Mutations are deferred to the main thread; element lookups return wrapper objects keyed
by element name. Colors are `[r, g, b, a]` arrays, positions/sizes are `[x, y]` arrays.

### Global functions

| Function               | Args              | Returns          | Description                                      |
|------------------------|-------------------|------------------|--------------------------------------------------|
| `FindUI(name)`         | string            | element or `nil` | Looks up an existing element by name.            |
| `CreateUIButton(name)` | string (optional) | button           | Creates a button (default name `"NewButton"`).   |
| `CreateUIText(name)`   | string (optional) | text             | Creates a text element (default `"NewText"`).    |
| `CreateUIRect(name)`   | string (optional) | rect             | Creates a rect element (default `"NewRect"`).    |
| `CreateUIImage(name)`  | string (optional) | image            | Creates an image element (default `"NewImage"`). |
| `DestroyUI(name)`      | string            | void             | Removes the element from its parent.             |

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

| Method                           | Args    | Returns              | Description                    |
|----------------------------------|---------|----------------------|--------------------------------|
| `GetText()`                      | -       | string               | Button label.                  |
| `GetTextColor()`                 | -       | array `[r, g, b, a]` | Text color.                    |
| `GetBackgroundColor()`           | -       | array `[r, g, b, a]` | Background color.              |
| `WasClicked()`                   | -       | bool                 | `true` if clicked this frame.  |
| `IsHovered()`                    | -       | bool                 | `true` while hovered.          |
| `IsHeld()`                       | -       | bool                 | `true` while held down.        |
| `SetText(text)`                  | string  | void                 | Sets the label.                |
| `SetTextColor(r, g, b, a)`       | numbers | void                 | Sets the text color.           |
| `SetBackgroundColor(r, g, b, a)` | numbers | void                 | Sets the background color.     |
| `SetFont(fontName)`              | string  | void                 | Sets the font by resource key. |
| `GetFont()`                      | -       | string               | The font resource key.         |

### Text methods

| Method                 | Args    | Returns              | Description                    |
|------------------------|---------|----------------------|--------------------------------|
| `GetText()`            | -       | string               | Text content.                  |
| `GetColor()`           | -       | array `[r, g, b, a]` | Text color.                    |
| `SetText(text)`        | string  | void                 | Sets content.                  |
| `SetColor(r, g, b, a)` | numbers | void                 | Sets color.                    |
| `SetFont(fontName)`    | string  | void                 | Sets the font by resource key. |

### Rect and Image methods

| Method                 | Args    | Returns              | Description         |
|------------------------|---------|----------------------|---------------------|
| `GetColor()`           | -       | array `[r, g, b, a]` | Element color.      |
| `SetColor(r, g, b, a)` | numbers | void                 | Sets element color. |

## See also

* [Getting started with ObSL](getting-started.md) lifecycle, hooks, and full examples.
* The language itself: [ObSL README](https://github.com/torkelicious/ObSL) and `external/obsl/docs/`.
* Source for this page: `src/Scripting/EngineLib/` (the `EngineLib*` module files).
