# Choroboros Build & Packaging Scripts

Run these from the project root, e.g. `./scripts/package.sh`.

| Script | Purpose |
|--------|---------|
| `build_macos_universal.sh` | Build universal macOS binary (Intel + Apple Silicon) |
| `package.sh` | Create distribution ZIP (VST3, AU, Standalone) |
| `create_dmg.sh` | Create DMG installer for distribution |
| `clear_plugin_cache.sh` | Clear DAW plugin caches |
| `reinstall_plugin.sh` | Remove old installs and install from `Release/` |
