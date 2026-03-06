# Choroboros Windows Release Hard Checklist

Release scope: VST3 + Standalone, Windows x64 primary, Windows x86 compatibility build.

Host scope for in-VM validation: REAPER x64 and FL Studio x64 only.
Ableton Live and other DAWs are covered by external beta testing.

## 0) Branch Integrity (Must Pass)

- [ ] `git fetch origin`
- [ ] `git status --porcelain` is empty before packaging
- [ ] `git rev-parse --short HEAD` captured in release notes
- [ ] `git log -1 --oneline` captured in release notes

## 1) Toolchain + Preflight (Must Pass)

- [ ] Run `powershell -ExecutionPolicy Bypass -File .\windows\preflight.ps1`
- [ ] CMake, Git, and MSVC toolchain all detected
- [ ] Build path is valid (no path length/toolchain issues)

## 2) Clean Build Reproducibility (Must Pass)

### x64 Release (primary)
- [ ] Configure:
  `cmake -S . -B build\windows-x64-release -G "Visual Studio 17 2022" -A x64`
- [ ] Build:
  `cmake --build build\windows-x64-release --config Release --target Choroboros_VST3 Choroboros_Standalone --parallel`

### x86 Release (compatibility)
- [ ] Configure:
  `cmake -S . -B build\windows-x86-release -G "Visual Studio 17 2022" -A Win32`
- [ ] Build:
  `cmake --build build\windows-x86-release --config Release --target Choroboros_VST3 Choroboros_Standalone --parallel`

## 3) Binary/Install Integrity (Must Pass)

- [ ] Install latest x64 build to:
  `C:\Program Files\Common Files\VST3\Choroboros.vst3`
- [ ] Optional user mirror:
  `C:\Users\<user>\AppData\Local\Programs\Common\VST3\Choroboros.vst3`
- [ ] Verify installed binary hash equals built binary hash
- [ ] Confirm no stale alternate copies are being loaded

## 4) Defaults Source-of-Truth Verification (Must Pass)

- [ ] Run:
  `powershell -ExecutionPolicy Bypass -File .\windows\trace_effective_defaults.ps1`
- [ ] `EffectiveUser` JSON is valid
- [ ] Diff vs repo source (`windows_factory_defaults.json`) is zero changed keys
- [ ] Probe values match expected release defaults

## 5) Crash Capture Readiness (Must Pass)

- [ ] Run setup:
  `powershell -ExecutionPolicy Bypass -File .\windows\capture_reaper_crash.ps1`
- [ ] LocalDumps configured for `reaper.exe` and `reaper_host64.exe`
- [ ] If a crash occurs, dump path + SHA256 are recorded

## 6) Stability Smoke in REAPER x64 (Must Pass)

- [ ] Fresh scan cache reset (`reaper-vstplugins64.ini` removed)
- [ ] Insert plugin in empty project: no crash
- [ ] Close/open plugin editor repeatedly: no crash
- [ ] Switch engines and HQ repeatedly: no crash
- [ ] Open Dev Panel and switch all tabs repeatedly: no crash
- [ ] Unlock warning appears on first unlock attempt
- [ ] Unlock warning supports "Hide this message in the future"
- [ ] Safety setting can re-enable warning (`Settings > Safety > Unlock Warning`)
- [ ] Console command `reset all` works as recovery path
- [ ] Save/reopen project with plugin instance: no crash and state intact

## 7) Stability Smoke in FL Studio x64 (Must Pass)

- [ ] Rescan plugins in Plugin Manager
- [ ] Insert plugin: no crash
- [ ] Open/close editor repeatedly: no crash
- [ ] Switch engines/HQ and adjust primary controls: no crash
- [ ] Save/reopen project with plugin instance: no crash and state intact

## 8) Performance Trace Capture (Required Evidence)

- [ ] Reset trace:
  `powershell -ExecutionPolicy Bypass -File .\windows\trace_load_performance.ps1 -Reset`
- [ ] Run controlled scenario (insert plugin, open panel, switch tabs, close DAW)
- [ ] Export summary:
  `powershell -ExecutionPolicy Bypass -File .\windows\trace_load_performance.ps1 -Tail 120 | Tee-Object .\windows\perf_windows_latest.txt`
- [ ] Record per-instance startup timeline in release notes

## 9) Packaging (Must Pass)

- [ ] Build zip for x64 release package
- [ ] Build zip for x86 compatibility package
- [ ] Packaging command executed:
  `powershell -ExecutionPolicy Bypass -File .\windows\package_windows_release.ps1 -Config Release -VersionLabel v2.03-beta`
- [ ] Include README/install instructions and host path notes
- [ ] Include commit SHA and build timestamp in release notes

## 10) Final Go/No-Go

- [ ] No host crashes in REAPER x64 smoke
- [ ] No host crashes in FL Studio x64 smoke
- [ ] Defaults trace and performance trace attached to release notes
- [ ] x64 marked as primary download; x86 marked as compatibility

---

## Current Branch Execution Log (Local Run)

Branch:
- [x] `main` synced with `origin/main` (checked on 2026-03-05 via `git status --porcelain --branch`)
- [x] Current base commit captured: `566f176`

Local compile sanity:
- [x] `cmake --build build/macos-ninja-debug --target Choroboros_VST3 Choroboros_Standalone --parallel`

Code-level safety gates:
- [x] Unlock warning is enforced for both "Unlock All" and per-control unlock
- [x] Unlock warning supports "Hide this message in the future"
- [x] Settings > Safety includes toggle to re-enable warning
