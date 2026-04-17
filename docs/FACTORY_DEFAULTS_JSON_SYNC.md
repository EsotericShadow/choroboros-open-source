# Factory defaults JSON — cross-platform parity

Four files in this repository must contain **identical** JSON bytes. If they diverge, macOS, Linux, and Windows builds embed **different** factory tuning/layout and you get unreproducible bug reports.

## The four files (paths from this repo root)

| File | Embedded resource (when applicable) |
|------|-------------------------------------|
| `Assets/defaults_factory_mac.json` | None — **editorial canonical**; edit here first. |
| `json_defaults_dump.json` | `json_defaults_dump_json` (macOS `BinaryData`) |
| `linux/linux_factory_defaults.json` | `linux_factory_defaults_json` |
| `windows/windows_factory_defaults.json` | `windows_factory_defaults_json` |

See `CMakeLists.txt` (`CHOROBOROS_BINARY_DATA_CORE_SOURCES`) and `Source/Plugin/PluginProcessor.cpp` (`seedPersistedDefaultsFromBundledFactory`).

## After any change to factory JSON

1. Edit **`Assets/defaults_factory_mac.json`** (recommended).
2. Copy it to the other three paths:

```bash
# From the choroboros-open-source directory (your clone root).
CANON="Assets/defaults_factory_mac.json"
cp "$CANON" "json_defaults_dump.json"
cp "$CANON" "linux/linux_factory_defaults.json"
cp "$CANON" "windows/windows_factory_defaults.json"
```

3. Verify **before commit or release**:

```bash
./scripts/verify_factory_json_sync.sh
```

Exit code **0** = all four match. Exit code **1** = `diff` printed; fix then re-run.

On macOS/Linux after CMake configure:

```bash
cmake --build build --target verify_factory_json_sync
```

(`verify_factory_json_sync` is defined under `if(UNIX)` in `CMakeLists.txt`.)

4. Rebuild so `BinaryData` picks up the updated JSON.

## `cd` to the repo root (examples)

Use the directory that actually contains `CMakeLists.txt` and `Assets/`:

- **Generic (any machine):** `cd /path/to/choroboros-open-source`
- **Primary dev layout (external T7 volume):** `cd /Volumes/T7/Users-main/Desktop/CHOROS_MASTER/choroboros-open-source`
- **Another common layout:** `cd ~/Desktop/CHOROS_MASTER/choroboros-open-source`

Only the prefix changes; commands above are always run from the **clone root**.

## Related

- `scripts/README.md` — script index including `verify_factory_json_sync.sh`
- `windows/RELEASE_CHECKLIST_HARD.md` — Windows hard checklist §4 includes the same four-file parity gate
- Broader workflow (runtime paths, Reaper refresh): `engineering/kb_refs/project_factory_defaults_update_workflow.md` (sibling `CHOROS_MASTER` tree, if you use that knowledge base)
