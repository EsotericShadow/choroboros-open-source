# Windows Prep Workspace

This directory is the Windows handoff and execution workspace for building and validating Choroboros on Windows (x64 primary, x86 optional).

It is designed so a new agent or engineer can open this folder and run predictable steps without reverse-engineering the project.

## What This Folder Is For

- One place for Windows-specific build scripts and runbooks.
- Consistent commands for Visual Studio / CMake on Windows.
- Preflight checks to fail fast before long builds.
- Clear host/plugin output and install paths.
- CI-safe regression test entrypoint (`--dsp-only`) to avoid GUI-runner issues.

## What This Folder Is Not

- Not a DSP refactor area.
- Not a parameter/state behavior change area.
- Not a replacement for project root build files.

## Quick Start (Windows PowerShell)

1. Open **Developer PowerShell for VS 2022** (recommended).
2. `cd` into repository root.
3. Run preflight:

```powershell
powershell -ExecutionPolicy Bypass -File .\windows\preflight.ps1
```

4. Build x64 (Release):

```powershell
powershell -ExecutionPolicy Bypass -File .\windows\build_windows_x64.ps1 -Config Release
```

5. Run regression harness in CI-safe mode:

```powershell
powershell -ExecutionPolicy Bypass -File .\windows\run_regression.ps1 -Config Release -DspOnly $true
```

6. Install VST3 to user path for DAW scan:

```powershell
powershell -ExecutionPolicy Bypass -File .\windows\install_vst3_user.ps1
```

7. Verify exactly which defaults JSON the runtime will use (and diff against repo source):

```powershell
powershell -ExecutionPolicy Bypass -File .\windows\trace_effective_defaults.ps1
```

8. Capture REAPER crash dump + Application log context (for tab/open crash triage):

```powershell
powershell -ExecutionPolicy Bypass -File .\windows\capture_reaper_crash.ps1
```

## Presets

Root `CMakePresets.json` includes:

- `windows-x64-release` (primary)
- `windows-x64-debug`
- `windows-x86-release` (legacy host compatibility checks only)

Build presets:

- `build-windows-x64-release`
- `build-windows-x64-regression`
- `build-windows-x86-release`

## Files in This Folder

- `AGENT_CONTEXT.md` - constraints, priorities, guardrails
- `CHECKLIST.md` - step-by-step verification checklist
- `HOST_PLUGIN_PATHS.md` - where outputs go and where hosts look
- `preflight.ps1` - toolchain/environment sanity checks
- `build_windows_x64.ps1` - primary build script
- `build_windows_x86.ps1` - optional x86 build script
- `run_regression.ps1` - regression execution helper
- `install_vst3_user.ps1` - install built VST3 to user plugin folder
- `trace_effective_defaults.ps1` - runtime-equivalent defaults source trace + flattened diff
- `capture_reaper_crash.ps1` - one-shot REAPER crash capture (LocalDumps + event log summary)
- `build_windows_x64.bat` / `build_windows_x86.bat` - wrappers for double-click flow

## Important Invariants

- Keep DSP/parameter-state behavior unchanged in Windows prep work.
- Any GUI/test split should be test-harness-only, not audio runtime logic.
- If adding new Windows toggles, default behavior must remain aligned with current shipping behavior.
