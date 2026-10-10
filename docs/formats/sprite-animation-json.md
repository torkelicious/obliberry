# Sprite Animation Sets (`assets/animations/*.json`)

A sprite animation set contains one sheet, and a collection of named "clips".

## Example

```json
{
    "sheet": {
        "texture_id": "player_sheet",
        "columns": 4,
        "rows": 2,
        "column_spacing": 0,
        "row_spacing": 0
    },
    "clips": {
        "idle": {
            "loop": true,
            "frames": [
                { "index": 0, "duration": 0.2 },
                { "index": 1, "duration": 0.2 }
            ]
        },
        "walk": {
            "loop": true,
            "frames": [
                { "index": 4, "duration": 0.1 },
                { "index": 5, "duration": 0.1 },
                { "index": 6, "duration": 0.1 },
                { "index": 7, "duration": 0.1 }
            ]
        }
    }
}
```

## Sheet

| Field            | Meaning                                       |
|------------------|-----------------------------------------------|
| `texture_id`     | Registered Texture Resource ID                |
| `columns`        | Number of columns; must be greater than zero. |
| `rows`           | Number of rows; must be greater than zero.    |
| `column_spacing` | Non-negative pixel gap between columns.       |
| `row_spacing`    | Non-negative pixel gap between rows.          |

Spacing describes a gap between cells. It is not an outer border of the sheet.

The available image width is:
`texture Width - column_spacing * (columns - 1)`

It must divide evenly into the columns, with at least one pixel per column.
The same applies to height, row spacing, and rows.

The texture ID must have an entry in the project [`assets.json`](assets-json.md). Lazy loading resolves and acquires
that
texture before deserializing the animation set.

## Clips

Each key in `clips` is a non empty clip name.

| Field               | Meaning                                        |
|---------------------|------------------------------------------------|
| `loop`              | Whether playback repeats after the last frame. |
| `frames`            | Non-empty array defining playback order.       |
| `frames[].index`    | Zero-based sheet frame index.                  |
| `frames[].duration` | Finite duration in seconds, greater than zero. |

Indices run left-to-right then top-to-bottom.
Each index must be less than `columns * rows`.

Frames can repeat or appear in any order, or have differing durations.

A uniform 10 FPS clip uses `0.1` seconds per frame.
FPS is more or less just an editor convenience, the file itself stores individual frame durations.

### Resource id

The animation resource ID and source path are stored in the project-root `assets.json` `animation_sets` array and are
not part of this file.

See [`assets.json`](assets-json.md) for registration and [Scene JSON](scene-json.md#asset-references) for component
references.

## Runtime state

This file stores animation definitions only. Current clip selection,
frame position, elapsed time, and playing state belong to each entity's
SpriteAnimator and are not stored here.
