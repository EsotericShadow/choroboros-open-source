# Windows Build and Validation Checklist

## 0) Preconditions

- [ ] Windows 10/11 x64 machine
- [ ] Visual Studio 2022 with C++ workload
- [ ] CMake 3.22+
- [ ] Git available
- [ ] Repository cloned locally

## 1) Preflight

- [ ] Run `windows/preflight.ps1`
- [ ] Confirm `cmake`, `git`, and MSVC toolchain are found
- [ ] Confirm build tree path is not excessively deep
- [ ] Confirm JUCE availability path or network fetch capability

## 2) Configure + Build (x64 primary)

- [ ] Configure with `windows/build_windows_x64.ps1 -Config Release`
- [ ] Build `Choroboros_VST3`
- [ ] Build `Choroboros_Standalone`
- [ ] Build `ChoroborosRegressionTests`

## 3) Regression

- [ ] Run DSP-safe regression:
  `windows/run_regression.ps1 -Config Release -DspOnly $true`
- [ ] Optional local full regression (GUI available):
  `windows/run_regression.ps1 -Config Release -DspOnly $false`

## 4) Install and Host Smoke

- [ ] Install VST3 to user path:
  `windows/install_vst3_user.ps1`
- [ ] Rescan in target DAW
- [ ] Plugin loads with no crash
- [ ] Front panel responds normally
- [ ] DEV panel opens and renders
- [ ] Engine/HQ switching works
- [ ] Console accepts commands

## 5) Release Parity Checks

- [ ] Preset names and load behavior match macOS
- [ ] Parameter display/edit behavior unchanged
- [ ] Core assignment UI appears and updates correctly
- [ ] Tutorial flow runs with no missing targets
- [ ] No obvious theming/font regressions

## 6) Packaging and Legal Transparency

- [ ] Output artifact paths documented in handoff
- [ ] License/notice bundle included as required
- [ ] No conflicting release-license statements in package docs

## 7) Exit Criteria

- [ ] x64 build reproducible on clean machine
- [ ] Regression harness passes in `--dsp-only` mode
- [ ] Plugin host smoke pass complete
- [ ] Blockers documented with exact errors and next step
