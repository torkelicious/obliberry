# Component Reference

Every visible thing in an Obliberry scene is an **entity** made of **components**. This page documents every component you can add in the Inspector, what it does, what it requires, and what fields are exposed in the editor UI.

> [!NOTE]
> Entities can only have **one of each component type**. However, components themselves can hold multiple values (e.g., a single `ScriptComponent` can reference multiple `.obsl` scripts).

---

## Core Components (Added Automatically)

When you create a new entity, it gets these three components by default. **Without all three, the entity will not render.**

### Transform

Position, rotation, scale in world space. Also controls billboard mode.

| UI Field | Type | Notes |
|----------|------|-------|
| **Position** | DragFloat3 (xyz) | World position |
| **Rotation** | DragFloat3 (xyz) | Euler angles in degrees. Disabled if "Use Billboard" is checked. |
| **Scale** | DragFloat3 (xyz) | Local scale |
| **Use Billboard** | Checkbox | Adds/removes `BillboardTagComponent`. When enabled, rotation has no effect (sprite always faces camera). |

---

### Mesh

The geometric shape of the entity.

| UI Field | Type | Notes |
|----------|------|-------|
| **Factory** | Read-only text | Shows the mesh factory ID (e.g., `Quad`, `Hexagon`, `Circle`, `Ring`, `PointTopHex`) |
| **Indices** | Read-only text | Triangle index count |
| **Mesh** | Combo box | Pick from registered mesh factories |
| **Remove Mesh** | Button | Removes the MeshComponent |

---

### Material

Appearance: texture, color, shader. Links to a material resource.

| UI Field | Type | Notes |
|----------|------|-------|
| **Material** | Combo box | Pick from project materials |
| **Color** | ColorEdit4 (RGBA) | Tint color |
| **Texture** | Combo box | Pick from project textures |
| **Shader** | Combo box | Pick from project shaders |
| **Shader Paths** | Read-only text | Shows vertex/fragment paths of selected shader |
| **Clone Material** | Button | Creates a copy of the current material with a new name |
| **Remove Material** | Button | Removes the MaterialComponent |

---

## Behavior Components

### Movement

Allows an entity to navigate the hex grid using the movement system.

| UI Field | Type | Notes |
|----------|------|-------|
| **Time Per Step** | Float | Seconds per hex step (default 0.15) |
| **Step Timer** | Float (read-only) | Internal timer for current step |
| **Idle Timer** | Float (read-only) | Time spent idle |
| **Is Moving** | Bool (read-only) | Whether entity is currently moving |
| **Path Nodes** | Read-only text | Number of hexes in current path |
| **Current Path Index** | Read-only text | Progress along path |

> [!NOTE]
> `Step Timer`, `Idle Timer`, `Is Moving`, `Path Nodes`, and `Current Path Index` are runtime state shown for debugging. Only **Time Per Step** is editable.

---

### DirectionalTexture

Swaps the entity's texture based on which direction it's facing (6 directions on a hex grid).

| UI Field | Type | Notes |
|----------|------|-------|
| **Facing Index** | Slider (0-5) | Current facing direction. 0-5 correspond to hex grid directions. |
| **Dir 0 Texture** ... **Dir 5 Texture** | 6× Combo boxes | Texture for each direction. Empty = unset. |
| **Missing count warning** | Read-only text | Shows how many directions have no texture assigned. |
| **Remove Directional Texture** | Button | Removes the component |

**Requires:** `TransformComponent`, `MovementComponent`, `MaterialComponent`

---

### Script

Attaches ObSL scripts to the entity. Scripts define custom logic.

| UI Field | Type | Notes |
|----------|------|-------|
| **Script list** | Bullet list | Each attached script path with a **Remove** button |
| **Add Script** | FileCombo | Pick a `.obsl` file from `assets/scripts/` |
| **Total count** | Read-only text | Number of attached scripts |

---

## Visual Effects Components

### ParticleEmitter

Emits particles from the entity's position.

| UI Section | Fields |
|------------|--------|
| **Emission** | **Active** (bool), **Editor Preview** (bool), **Max Particles** (int, 1-16384), **Emit Rate** (float, particles/sec) |
| **Lifetime** | **Min** / **Max** (float, seconds) |
| **Velocity** | **Min** / **Max** (DragFloat3, vec3) |
| **Physics** | **Gravity** (DragFloat3, vec3) |
| **Size** | **Start Min** / **Start Max**, **End Min** / **End Max** (float) |
| **Rotation** | **Speed Min** / **Speed Max** (float, degrees/sec) |
| **Color** | **Start** / **End** (ColorEdit4, RGBA) |
| **Options** | **Billboard** (bool), **Blend Mode** (Alpha / Additive), **Render Order** (int, -10 to 10), **Shape** (Quad / Circle / Soft Circle) |
| **Material** | Combo box to pick material (must have texture for visible particles) |
| **Presets** | **Load Preset** (FileCombo from `assets/particles/`), **Save as Preset** (button + popup) |
| **Remove** | Button to remove component |

**Requires:** `TransformComponent`, `MaterialComponent`

---

### PointLight

A dynamic point light that affects the lighting system.

| UI Field | Type | Notes |
|----------|------|-------|
| **Color** | Color3 (RGB) | Light color |
| **Radius** | Float | World-space radius |
| **Intensity** | Float | Light intensity |

**Requires:** `TransformComponent`

---

## Map Components

These appear on the **MAP** entity (created automatically in Map Edit mode).

### Map

| UI Field | Type | Notes |
|----------|------|-------|
| **Map File** | FileCombo | Pick `.obmap` file from `assets/maps/` |
| **Needs Mesh Update** | Checkbox | Triggers mesh rebuild |
| **Render Visibles** | Read-only text | Number of visible tile types |
| **Tile Types** | Read-only text | Number of defined tile types |
| **Hex Mesh** | Read-only text | Loaded / Missing |
| **Overlay materials** | Read-only text | Warning if selection/path materials missing |
| **Remove Map** | Button | Removes MapComponent (and MapStateComponent) |

### Map State

Runtime state for map editing (selection, pathfinding).

| UI Field | Type | Notes |
|----------|------|-------|
| **Has Selection** | Checkbox | Whether a hex is currently hovered/selected |
| **Selected Hex** | Read-only text | Coordinates `[q, r]` of selected hex |
| **Has Path To** | Checkbox | Whether a path target is set |
| **Path Target** | Read-only text | Coordinates `[q, r]` of path target |
| **Remove Map State** | Button | Removes MapStateComponent |

---

## Internal / Tag Components

These are used by the engine internally. You generally **don't add them manually** - they're assigned automatically or shown as tag flags in the Inspector.

| Tag | How It Appears in UI |
|-----|----------------------|
| **BillboardTag** | As **"Use Billboard"** checkbox in Transform widget |
| **DestroyTag** | Not shown (entity queued for deletion) |
| **Relationship** | Managed via Registry context menu (Create Child, Set Parent, Detach) |
| **CustomData** | Shows as **ObSL Custom Data** widget (read-only list of script-defined variables) |
| **PrefabSource** | Added automatically when dragging a prefab in. Shows "Select Prefab Source" in Registry context menu. |
| **MapState** / **Map** | On the MAP entity (see above) |

---

## UI Components

UI elements (Text, Button, Image, Rect) are managed in the **UI Hierarchy** panel, not the entity Registry. They have their own properties:

| Element Type | Key Properties |
|--------------|----------------|
| **Text** | `text`, `color` (RGBA), `font` (font resource ID) |
| **Button** | `text`, `color`, `bg_color`, `hovered_bg_color` (RGBA), `bg_texture` (optional), `font` (optional) |
| **Image** | `texture` (texture resource ID), `color` (RGBA) |
| **Rect** | `color` (RGBA) |

All UI elements share: `name`, `type`, `rect.position` (x, y), `rect.scale` (x, y), `flags` (bit 0 = visible, bit 1 = enabled), `children` (array).

---

## Quick Reference Table

| Category | Components |
|----------|------------|
| **Required for rendering** | Transform, Mesh, Material |
| **Movement & AI** | Movement, DirectionalTexture |
| **Logic** | Script |
| **Visual effects** | ParticleEmitter, PointLight |
| **Map** | Map, Map State (on MAP entity) |
| **Internal tags** | BillboardTag (via Transform), DestroyTag, Relationship, CustomData, PrefabSource |
| **UI** | Text, Button, Image, Rect (in UI Hierarchy) |

---

## Common Patterns

| Goal | Components to Add |
|------|-------------------|
| Static prop (crate, rock) | Transform + Mesh + Material |
| Moving character | Transform + Mesh + Material + Movement + Script |
| Directional sprite (tank, NPC) | Transform + Mesh + Material + Movement + DirectionalTexture + Script |
| Light source | Transform + PointLight |
| Particle effect (fire, magic) | Transform + Material + ParticleEmitter |
| Billboard sprite (tree, particle) | Transform + Mesh + Material (check "Use Billboard" in Transform) |

