# Windows Host and Plugin Paths

## Build Artifacts (default from scripts)

### x64 Release

- Build root: `build/windows-x64-release`
- VST3 bundle (typical):
  - `build/windows-x64-release/Choroboros_artefacts/Release/VST3/Choroboros.vst3`
- Standalone app (typical):
  - `build/windows-x64-release/Choroboros_artefacts/Release/Standalone/Choroboros.exe`
- Regression test exe (typical):
  - `build/windows-x64-release/Release/ChoroborosRegressionTests.exe`

### x86 Release (optional)

- Build root: `build/windows-x86-release`
- Same relative artifact structure as x64.

## Common Windows VST3 Scan Paths

- System-wide:
  - `C:\Program Files\Common Files\VST3`
- Per-user:
  - `%LOCALAPPDATA%\Programs\Common\VST3`

The helper script installs to the per-user path by default.

## DAW Notes

- Reaper: confirm VST3 path in Preferences -> Plug-ins -> VST.
- Ableton Live: enable VST3 in Preferences -> Plug-ins.
- Studio One / Cubase: rescan plugin folders after copy.
- Pro Tools (AAX): separate AAX path and validation flow required.

## Troubleshooting

- If plugin does not appear:
  - verify exact bundle path
  - delete stale prior version
  - force DAW rescan
  - restart DAW
- If host blacklists plugin:
  - inspect plugin scanner logs
  - run regression harness first to ensure local binary sanity
