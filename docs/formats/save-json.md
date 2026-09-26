# Save format

Saves are JSON files containing metadata and a project-defined key/value store.

This does **not** automatically serialize scenes, entities, components, or script variables. ObSL scripts
explicitly choose the values needed by their game and restore the corresponding game state after loading them.

## Storage directory

Save files are stored below the directory selected by `saves.location` in [`project.json`](project-json.md):

```text
<save home>/obliberry/<project UUID>/saves
```

The save directory is created when an enabled save system is configured.

## Filenames

New saves use a UTC millisecond timestamp:

```text
save-<timestamp>.json
```

If that name already exists, the manager adds a numeric suffix:

```text
save-<timestamp>-1.json
save-<timestamp>-2.json
```

Script file operations accept only a filename beginning with `save-` and ending in `.json`. Absolute paths and paths
containing parent directories are rejected.

## Example

```json
{
    "version": 1,
    "name": "Before doing a thing",
    "created_at": 1790448000000,
    "updated_at": 1790448065000,
    "values": {
        "health": 50,
        "something.complete": true,
        "player.name": "Glorb"
    }
}
```

## Fields

| Key          | Type    | Meaning                                                              |
| ------------ | ------- | -------------------------------------------------------------------- |
| `version`    | integer | Save-format version. The current and supported version is `1`.       |
| `name`       | string  | User-facing display name supplied to `save_create`.                  |
| `created_at` | integer | Creation time in UTC milliseconds since the Unix epoch.              |
| `updated_at` | integer | Last successful write time in UTC milliseconds since the Unix epoch. |
| `values`     | object  | Key/value data explicitly stored through the ObSL save API.          |

Stored values may be booleans, signed integers, finite floating-point numbers, or strings. ObSL represents all numbers
as doubles, so integers read by a script are exposed as ObSL numbers.

Unknown save versions, missing required fields, empty value keys, unsupported JSON value types, and non-finite numbers
cause the file to be rejected.

## Lifecycle

1. `save_new()` clears the in-memory data and active filename.
2. Scripts populate data with `save_set(key, value)`.
3. `save_create(displayName)` creates a new file and makes it active.
4. Later changes are persisted to that file with `save_write()`.
5. `save_list()` enumerates valid saves, newest `updated_at` first.
6. `save_load(filename)` replaces the active in-memory data with the selected file.
7. `save_delete(filename)` deletes the selected file. Deleting the active file also clears its active filename.

Files are written through a temporary file and then replaced, reducing the chance of leaving a partially written save.

See the [ObSL API reference](../scripting/api-reference.md#save-data) for every script function.
