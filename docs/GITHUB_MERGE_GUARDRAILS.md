# GitHub Merge Guardrails – Deep Analysis & Verification

**Purpose:** Prevent regressions when merging local (v2.01-beta) with remote (origin/main). Run all checks before and after merge.

---

## 1. BinaryData / Asset Verification

### Code References vs CMakeLists

Every `BinaryData::*` reference in Source/ must have a matching asset in `CMakeLists.txt` → `juce_add_binary_data(ChoroborosBinaryData SOURCES ...)`.

| BinaryData Symbol | Source File | CMakeLists Asset | Status |
|-------------------|-------------|------------------|--------|
| green_backpanel_png | PluginEditor.cpp | Assets/green/green_backpanel.png | ✅ |
| blue_backpanel_png | PluginEditor.cpp | Assets/blue/blue_backpanel.png | ✅ |
| red_backpanel_png | PluginEditor.cpp | Assets/red/red_backpanel.png | ✅ |
| purple_backpanel_png | PluginEditor.cpp | Assets/purple/purple_backpanel.png | ✅ |
| black_backpanel_png | PluginEditor.cpp | Assets/black/black_backpanel.png | ✅ |
| Technology_ttf | PluginEditor.cpp | Assets/fonts/Technology.ttf | ✅ |
| Retroica_ttf | PluginEditor, FeedbackDialog, DevPanel | Assets/fonts/Retroica.ttf | ✅ |
| EULA_md | AboutDialog.cpp | EULA.md | ✅ |
| red_slider_thumb_png | CustomLookAndFeel.cpp | Assets/red/red_slider_thumb.png | ✅ |
| rate/depth/offset/width_knob_spritesheet_png | CustomLookAndFeel (green) | Assets/green/*.png | ✅ |
| mix_knob_spritesheet_png | CustomLookAndFeel (green) | Assets/green/mix_knob_spritesheet.png | ✅ |
| Blue_knob_spritesheet_png | CustomLookAndFeel (blue) | Assets/blue/Blue_knob_spritesheet.png | ✅ |
| blue_mix_knob_spritesheet_png | CustomLookAndFeel (blue) | Assets/blue/blue_mix_knob_spritesheet.png | ✅ |
| red_dragon_knob_png | CustomLookAndFeel (red) | Assets/red/red_dragon_knob.png | ✅ |
| red_mix_knob_spritesheet_png | CustomLookAndFeel (red) | Assets/red/red_mix_knob_spritesheet.png | ✅ |
| purple_knob_spritesheet_png | CustomLookAndFeel (purple) | Assets/purple/purple_knob_spritesheet.png | ✅ |
| purple_mix_knob_spritesheet_png | CustomLookAndFeel (purple) | Assets/purple/purple_mix_knob_spritesheet.png | ✅ |
| black_Knob_spritesheet_png | CustomLookAndFeel (black) | Assets/black/black_Knob_spritesheet.png | ✅ |
| black_mix_knob_spritesheet_png | CustomLookAndFeel (black) | Assets/black/black_mix_knob_spritesheet.png | ✅ |
| switch_a_spritesheet_png | AnimatedToggleButton.cpp | Assets/switch_a_spritesheet.png | ✅ |
| indicator_north_facing_png | CustomLookAndFeel (blue) | Assets/blue/indicator_north_facing.png | ✅ |
| indicator_north_facing_png2 | CustomLookAndFeel (purple, black) | Assets/purple/indicator_north_facing.png | ✅ |
| indicator_shadow_overlay_png | CustomLookAndFeel (blue) | Assets/blue/indicator_shadow_overlay.png | ✅ |
| indicator_shadow_overlay_png2 | CustomLookAndFeel (red) | Assets/red/indicator_shadow_overlay.png | ✅ |
| indicator_shadow_overlay_png3 | CustomLookAndFeel (purple, black) | Assets/purple/indicator_shadow_overlay.png | ✅ |

**JUCE BinaryData naming:** Duplicate filenames get `_png2`, `_png3` suffixes. Order in CMakeLists determines which file gets which suffix.

---

## 2. Regression Test Verification

### ChoroborosRegressionTests.cpp

- **Parameter indices:** `getParameters()[0]`=Rate, `[5]`=Engine, `[6]`=HQ
- **Engine range:** `(i % 5) / 4.0f` → 0..1 for 5 engines (Green, Blue, Red, Purple, Black)
- **Tests:** processBlock sizes, Engine/HQ torture, state round-trip, max block channels

**Pre-merge check:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target ChoroborosRegressionTests
./build/ChoroborosRegressionTests
# Expected: "PASS: All regression tests passed."
```

**Status:** ✅ Verified passing (2024-02-24)

---

## 3. Remote CI Workflow Requirements

### build-linux.yml / build-all-platforms / build-windows

| Requirement | Remote Expectation | Local Status |
|-------------|--------------------|--------------|
| JUCE | In repo (vendored) | Gitignored – **must fix** |
| Build dir | `Linux-Build`, `Universal-Build`, `Windows-Build` | Local uses `build/` – workflows use different names |
| Artefacts path | `*/Choroboros_artefacts/Release/VST3` | Same (JUCE default) |
| DISTRIBUTION.md | At repo root | Local: `docs/archive/DISTRIBUTION.md` – **must copy or symlink** |
| EULA.md | At repo root | ✅ Present |
| README.md | At repo root | ✅ Present |

### Dockerfile.linux

- `COPY . .` – expects JUCE in repo
- `cmake -B Linux-Build` – uses Linux-Build, not build/
- `cp README.md DISTRIBUTION.md EULA.md Release/` – DISTRIBUTION.md must exist at root

---

## 4. Source File Diff Summary

### Local-only (must NOT be overwritten by remote)

| Path | Purpose |
|------|---------|
| Source/Config/DefaultsPersistence.cpp | Layout/tuning persistence |
| Source/Config/DefaultsPersistence.h | |
| Source/Cores/black_engine_linear/* | Black engine (4 files) |
| Source/UI/DevPanel.cpp | Dev panel UI |
| Source/UI/DevPanel.h | |
| Tests/ChoroborosRegressionTests.cpp | Regression harness |

### Remote-only (bring in on merge)

| Path | Purpose |
|------|---------|
| .github/workflows/*.yml | CI/CD |
| Dockerfile.linux | Linux build |
| Dockerfile.linux-test | |
| .dockerignore | |
| GPL_COMPLIANCE_*.md | GPL compliance docs |
| GITHUB_ACTIONS_*.md | CI docs |
| GITHUB_BILLING_FIX.md | |
| DISTRIBUTION.md | At root (local has in docs/archive/) |

### Conflict resolution rule

- **Source/**, **Assets/**, **CMakeLists.txt**, **.gitignore** → **keep local**
- **.github/**, **Dockerfile***, **GPL_***, **GITHUB_*** → **keep remote**
- **DISTRIBUTION.md** → Copy `docs/archive/DISTRIBUTION.md` to root (or keep remote’s root copy)

---

## 5. Pre-Merge Checklist

- [ ] Run `./build/ChoroborosRegressionTests` – must pass
- [ ] Run `cmake --build build` – must succeed
- [ ] Verify all assets in CMakeLists exist on disk
- [ ] Decide JUCE strategy: submodule vs vendored vs build docs
- [ ] Ensure DISTRIBUTION.md at root for CI (copy from docs/archive if needed)
- [ ] Create merge branch: `git checkout -b merge-from-remote`

---

## 6. Post-Merge Verification

- [ ] `git submodule update --init --recursive` (if using submodule)
- [ ] `cmake -B build -DCMAKE_BUILD_TYPE=Release`
- [ ] `cmake --build build`
- [ ] `./build/ChoroborosRegressionTests`
- [ ] Load plugin in Reaper – verify UI, all 5 engines, no missing assets
- [ ] Push to feature branch first; run GitHub Actions before merging to main

---

## 7. Known Pitfalls

1. **JUCE symlink:** Local `JUCE` → `/Users/main/JUCE`. Do not commit. Replace with submodule or vendor.
2. **BinaryData order:** Changing asset order in CMakeLists can change `_png2`/`_png3` suffixes and break CustomLookAndFeel.
3. **Parameter indices:** Regression tests hardcode `[0]`, `[5]`, `[6]`. Do not reorder parameters without updating tests.
4. **Remote has no Tests target:** Remote CMakeLists has no `ChoroborosRegressionTests`. Local does. Keep local CMakeLists.
