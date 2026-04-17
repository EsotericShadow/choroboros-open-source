# Choroboros Build & Packaging Scripts

Run these from the project root, e.g. `./scripts/package.sh`.

| Script | Purpose |
|--------|---------|
| `build_macos_universal.sh` | Build universal macOS binary (Intel + Apple Silicon) |
| `package.sh` | Create distribution ZIP (VST3, AU, Standalone) |
| `create_dmg.sh` | Create DMG installer for distribution |
| `clear_plugin_cache.sh` | Clear DAW plugin caches |
| `reinstall_plugin.sh` | Remove old installs and install from `Release/` |
| `refresh_reaper_beta.sh` | Rebuild current beta VST3 and AU, reinstall the user-level `Choroboros Beta` bundles, and clear only its REAPER cache entries |
| `verify_factory_json_sync.sh` | **Release / CI gate:** fail if `Assets/defaults_factory_mac.json`, `json_defaults_dump.json`, `linux/linux_factory_defaults.json`, and `windows/windows_factory_defaults.json` are not byte-identical (prevents cross-platform factory drift). Run from repo root. CMake: `cmake --build build --target verify_factory_json_sync` (Unix). See `docs/FACTORY_DEFAULTS_JSON_SYNC.md`. |
| `trace_load_performance_macos.sh` | Summarize startup/load timing trace (`load_trace.ndjson`) on macOS |
