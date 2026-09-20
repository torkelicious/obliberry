# Sprite Animation

Sprite animation assets contain a sheet and named clips.
Entities reference these assets through their SpriteAnimator Component.

## Create Sprite Animation Set

1. Import the spritesheet texture through the Project Browser.
2. Open the Project Browser's Animations section.
3. Choose New Animation.
4. Enter a unique resource ID (basically name) and choose a JSON destination inside the project.
5. Select the texture in the animation editor.
6. Set the columns, rows, and pixel spacing.
7. Add a named clip and configure its frames.
8. Preview the clip and save the animation asset.

The resource ID identifies the asset in scenes. The project-wide `assets.json` catalog maps that ID to the animation
file path.

## Importing sets

Import its referenced texture first, then use Import Animation.

The file's `texture_id` must match a registered texture resource ID.
Importing an animation does not automatically import its texture.

Use Edit on an animation entry to open its editor.

## Sheet Settings

Columns and rows divide the image into equal-sized cells. Column and row
spacing specify pixel gaps between cells.

After subtracting the gaps, the image dimensions must divide evenly into
the grid. Frames are numbered from zero, left-to-right and top-to-bottom.

Temporarily invalid values can remain in the draft while editing.
Preview and saving require valid data.

## Clips and Frames

Each clip has a unique name, looping setting, and an ordered frame list.

A frame selects a sheet index and specifies a duration in seconds.
Indices can repeat, and individual frames can have different durations.

Use the preview's Play, Pause, and Restart controls to inspect playback.
The preview has its own playback state.

### Adding frames in batches

Enter the first and last sheet indices and an FPS value.

- Append Range adds the generated sequence to the clip.
- Replace Frames replaces its existing sequence.
- Both endpoints are included.
- A descending range creates a reversed sequence.
- Apply FPS to All Frames sets every frame's duration to `1 / FPS`.

Expand individual frame rows to change an index or duration, move a frame
up or down, duplicate it, or remove it. Expand All and Collapse All control
the frame list.

## Assign to an entity

1. Add a SpriteSheet component if needed.
2. Use Add Animator in its inspector widget.
3. Select the registered animation set.
4. Select an initial clip.
5. Enable Autoplay if playback should start automatically.
6. Save the scene.

Scripts can control playback through the SpriteAnimator wrapper.
See the [EngineLib API reference](../scripting/api-reference.md).

## Saving

The animation editor edits a draft. Saving writes the animation file and creates or updates its entry in
[`assets.json`](../formats/assets-json.md).

Save the scene after assigning an animation so the entity's `animation_id` is retained. Registering or editing the
animation definition itself is persisted by the animation editor, not by scene saving. Saving a scene does not save an
unfinished animation draft.

You should save before opening another animation or leaving edit mode!

Renaming or deleting clips does not automatically update clip names stored
on entities or in scripts. Update those references yourself.

See [Sprite Animation JSON](../formats/sprite-animation-json.md)
for the file format.
