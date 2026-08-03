# Scripting with ObSL : Getting Started

Obliberry games are scripted with **ObSL**, a small, interpreted, dynamically typed language that ships as a submodule
at `external/obsl`. Created for this engine.
This guide covers how the engine runs scripts; the language itself is documented in
the [ObSL README](https://github.com/torkelicious/ObSL) (`external/obsl/README.md`), with
an [architecture overview](https://github.com/torkelicious/ObSL/blob/master/docs/ARCHITECTURE.md) and
a [standard library reference](https://github.com/torkelicious/ObSL/blob/master/docs/STANDARD_LIBRARY.md).

For a full list of engine functions available to scripts, see the [API Reference](api-reference.md).

## How scripts attach to a scene

Scripts are stored as `.obsl` files, conventionally under `assets/scripts/` inside a project. They are attached to
entities through the `ScriptComponent` in the editor via the Inspector, or by hand in the scene file:

```json
{
    "components": {
        "ScriptComponent": {
            "scriptPath": "assets/scripts/PlayerMovement.obsl"
        }
    }
}
```

An entity can have multiple scripts (the scene format also accepts a `scriptPaths` array). Each script on each entity
gets its own instance.

## The script lifecycle

A script attached to an entity runs like this:

1. **Top level code runs once** when the script is loaded (when the scene loads, or when the entity is created). Use
   this for setup: `var` declarations, cached lookups, prints.
2. **`on_update(dt)`** if defined, it is called every frame with the raw frame delta time in seconds.
3. **`on_destroy()`** called when the entity is marked for destruction (has a `DestroyTagComponent`).
4. **`on_exit()`** called when the scene exits.

```obsl
println "script loaded";          // once, at load

var myValue = 10;                 // setup state

fn on_update(dt) {                // every frame
    myValue = myValue + 1;
}

fn on_destroy() {                 // entity destroyed
    println "goodbye";
}
```

### The `this` binding

Inside a script, the global **`this`** is an *entity object* referring to the entity the script is attached to. Use it
to reach components:

```obsl
fn on_update(dt) {
    var tf = this.GetComponent("Transform");
    if (tf != null) {
        var pos = tf.GetPosition(); // [x, y, z]
        tf.SetPosition(pos[0], pos[1] + 10.0 * dt, pos[2]);
    }
}
```

Entity objects expose `id` (a number you can pass to functions like `SetPathToHex`) and `name`, plus methods like
`SetName`, `GetChildren`, `Find`, and `AddCustomComponent`.
See [Registry : entity objects](api-reference.md#entity-objects).

### `dt` and time

* The `dt` passed to `on_update` is the **raw** delta time (not scaled by time scale).
* `get_dt()` returns the *time-scaled* delta (`deltaTime * timeScale`), useful inside helper functions that don't
  receive `dt`.
* See the [Time module](api-reference.md#time) for `GetRawDt`, `GetTimeScale`/`SetTimeScale`, and `GetFrameCount`.

### Hot reload

When running from a loose project (not from a packaged `.obpak`), the engine checks script source files for changes
every 300 frames and **hot-reloads** modified scripts the top-level code runs again and `on_update` continues with the
new version. Good for iterating in the editor.

### Threading and safety

Scripts execute in **parallel** across interpreter workers. The EngineLib API handles the details for you:

* Reads (components, input, camera, …) are safe and can be done anywhere.
* Mutations (component writes, entity creation/destruction, UI changes) are *deferred*: they are queued and applied on
  the main thread after the parallel script pass. You don't need to do anything just call the API.

## Player movement

The example below is adapted from `src/Scripting/EngineLib/Examples/PlayerMovement.obsl`. It assumes the entity has a
`MovementComponent`, and that a hex map exists in the scene:

```obsl
fn on_update(dt) {
    var movement = this.GetComponent("Movement");

    if (Input_IsMousePressed(0)) {
        var selection = GetSelectedHex();          // {hasSelection, q, r}
        if (selection.hasSelection) {
            SetPathToHex(this.id, selection.q, selection.r);  // A* path + start moving
        }
    }

    if (movement != null) {
        if (movement.GetIsMoving() == false) {
            ClearPathTarget();                     // arrived: clear the path overlay
        }
    }
}
```

* `GetSelectedHex()` returns the hex that the map's selection state currently points at
* `SetPathToHex(entityId, q, r)` runs A* pathfinding on the map grid from the entity's current hex toward `(q, r)`,
  stores the path, and starts the `MovementSystem` walking the entity along it.
* The engine draws a path overlay on the map (`hasPathTo`), which is why the script clears it once movement finished.

## Camera controls

Adapted from `src/Scripting/EngineLib/Examples/InteractionSystem.obsl` a free camera with WASD + edge panning, scroll
zoom, and hex hover selection:

```obsl
var ZOOM_SPEED = 0.2;
var PAN_SPEED = 15.0;
var EDGE_MARGIN = 20.0;

fn on_update(dt) {

    // scroll to zoom, clamped between 0.5 and 5.0
    var scroll_y = Input_GetScrollY();
    if (scroll_y != 0.0) {
        var zoom = Camera_GetZoom() + (scroll_y * ZOOM_SPEED);
        if (zoom < 0.5) { zoom = 0.5; }
        if (zoom > 5.0) { zoom = 5.0; }
        Camera_SetZoom(zoom);
    }

    // WASD panning + pan when the mouse is near a window edge
    var pan_x = 0.0;
    var pan_y = 0.0;
    if (Input_IsKeyDown("W")) { pan_y = pan_y + 1.0; }
    if (Input_IsKeyDown("S")) { pan_y = pan_y - 1.0; }
    if (Input_IsKeyDown("A")) { pan_x = pan_x - 1.0; }
    if (Input_IsKeyDown("D")) { pan_x = pan_x + 1.0; }

    var mouse_x = Input_GetMouseX();
    var mouse_y = Input_GetMouseY();
    if (mouse_x <= EDGE_MARGIN)              { pan_x = pan_x - 1.0; }
    if (mouse_x >= Window_GetWidth() - EDGE_MARGIN) { pan_x = pan_x + 1.0; }
    if (mouse_y <= EDGE_MARGIN)              { pan_y = pan_y + 1.0; }
    if (mouse_y >= Window_GetHeight() - EDGE_MARGIN) { pan_y = pan_y - 1.0; }

    // normalize diagonals
    if (pan_x != 0.0 and pan_y != 0.0) {
        pan_x = pan_x * 0.7071;
        pan_y = pan_y * 0.7071;
    }

    if (pan_x != 0.0 or pan_y != 0.0) {
        // Camera_PanScreenSpace handles zoom scaling and the rotated camera axes
        Camera_PanScreenSpace(pan_x * PAN_SPEED * dt, pan_y * PAN_SPEED * dt);
    }

    // hover highlight: convert the mouse position to a hex and select it
    var mouseWorld = Input_GetMouseWorldPos();   // {x, y} in world space
    var hovered = Math_WorldToHex(mouseWorld.x, mouseWorld.y);  // {q, r}
    SetSelectedHex(hovered.q, hovered.r);
}
```

Notes:

* `Camera_PanScreenSpace(dx, dy)` takes a screen-space delta and converts it to world movement, compensating for zoom
  and the camera's rotation, it's the right tool for player-controlled cameras.
* `Input_GetMouseWorldPos()` returns world coordinates under the cursor; in the editor it accounts for the viewport
  framebuffer, in the runtime it uses the window size.
* Key names are strings resolved through the input manager's key mappings (e.g. `"Space"`, `"Esc"`, `"W"`). Mouse
  buttons are numbers: `0` = left, `1` = right, `2` = middle.

## More examples

The engine ships a set of example scripts in `src/Scripting/EngineLib/Examples/`:

* `PlayerMovement.obsl` hex movement via click-to-move.
* `InteractionSystem.obsl` camera controls + hover selection.
* `UITest.obsl` create a UI button and set its text.
* `examples.obsl` an API showcase covering entities, components, audio, input, hex, camera, and scene loading. As its
  header warns, it's a showcase rather than plug-and-play code check the [API reference](api-reference.md) for the
  current names.
* `engine_integ_test.obsl` a larger test exercising many EngineLib functions.

## A minimal UI example

```obsl
var btn = CreateUIButton("newbtn");
btn.SetText("This is a button");
btn.SetPosition(100.0, 50.0);
btn.SetSize(200.0, 60.0);

fn on_update(dt) {
    if (btn.WasClicked()) {
        println "button clicked!";
    }
}
```

See the [UI module](api-reference.md#ui-gui) for the full element API.
