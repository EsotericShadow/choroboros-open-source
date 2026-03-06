# Changelog

All notable changes to Choroboros are documented here.

## [2.02.2] - 2026-03-04

### Added
- **Dev Panel Console:** Fully interactive command-line interface replacing static recent touches log.
- **Console Interface:** 30+ power user commands including `engine`, `hq`, `set`, `get`, `add`, `sub`, `macro`, `toggle`, `sweep`, `undo`, `redo`, `history`, `solo`, `watch`, `dump`, `diff factory`, `list`, `stats`, and aliases.
- **Interactive Tutorials:** Guided HUD overlay teaching analog DSP concepts (BBD whine, Tape wow/flutter, Phase Width) and dev panel validation workflows.
- **UI Settings Panel:** Surgical Dev Panel preferences including scalable text, color-vision assistance modes, reduced motion, focus rings, custom accent overrides, and safe-reset toggles.

### Changed
- **Dev Panel Layout:** Rebuilt Modulation layout into unified sections; reorganized Look & Feel engine groups; updated Tone tabs to "Engine Response"; simplified fixed-height inspectors.
- **Repo Architecture:** Dropped JUCE submodule in favor of lightweight `FetchContent` to reduce repository bloat.
- **DSP transactions:** Update skips during preset/state/profile transactions so audio thread won't consume intermediate parameter steps mid-switch.
- **Visuals:** Overview and Engine signal-flow Core rows now display exact algorithm names (e.g. "Lagrange 5th", "Thiran Allpass", "Phase Warp").

### Fixed
- **Engine switch artifacts:** Transactional profile apply with guard to prevent partial-state feedback.
- **Engine switch artifacts:** Adaptive warmup and crossfade duration (severity-based).
- **Engine switch artifacts:** Dual old/new rendering during crossfade—old core rendered with frozen old params.
- **Engine switch artifacts:** Old-path LFO phase continuation.
- **Manual Accent:** Manual accent color setting now properly overrides theme resolution rather than being neutralized.
- **Console Sync:** Fixed value text inputs/readouts to follow active engine accent dynamically instead of hardcoded default.
- **AU validation (Logic Pro):** Engine selection parameter now reports `isMetaParameter() = true` (from 2.02.1).

---

## [2.02.1] - 2026-02-26

### Fixed

- **AU validation (Logic Pro):** Engine selection parameter now reports `isMetaParameter() = true`, fixing AU validation failure on macOS Tahoe.

### Chore

- Binary distribution moved to GitHub Releases (fixes 4MB Vercel Blob truncation for large zip).

---

## [2.02-beta] - 2026-02-28

### Changed

- **Engine/dev-panel coherence:** Audit and align engine internals wiring with Dev Panel exposure across faces/HQ-NQ modes
- **BBD filtering:** Remove legacy bbdFilterPoles end-to-end; integrate BBD 5th-order cascade filtering path and dynamic cutoff redesign
- **Color/saturation routing:** Engine "color" modifiers affect wet path only; correct Red HQ/NQ color behavior and tooltip semantics
- **Engine switching:** Reduce artifacts with transition smoothing/crossfade handling; per-engine-color parameter memory (store/restore on return)
- **Typed value edit:** Inverse-map displayed values back to raw APVTS space before write; relax parser hard caps against tuned/displayed ranges
- **Rate quantize menu:** Musical subdivisions (Straight/Triplet/Dotted), cap quantized targets at 20 Hz
- **UI theming:** Plugin dropdown/popup/context/tooltip/callout surfaces match plugin aesthetic; About/Feedback dialogs restyled (Dev Panel excluded)

### Fixed

- Red NQ defaults set to tuned profile values
- Persist/copy/load new BBD-related internals consistently (stages, filter ratio, LPF fields)

### Chore

- Gitignore large third-party docs (PDFs, .url) in Documentation/

---

## [2.01-beta] - 2026

### Added

- **Black engine:** Fifth engine with linear interpolation (Normal) and Linear Ensemble (HQ)
- **Engine-specific slider layout:** Per-engine X, Y, and size controls for the color slider in Dev Panel
- **Defaults persistence:** "Set Current as Defaults" saves layout, tuning, and DSP internals to startup defaults
- **Feedback system:** Beta feedback dialog with usage stats, save-to-file, and email options
- **Beta sign-up link:** Google Form for beta tester registration
- **HQ switch inversion:** Switch up = HQ on, switch down = HQ off (corrected mapping)
- **Per-engine slider thumbs:** Green, Blue, Red, Purple, and Black each have dedicated slider thumb assets

### Changed

- Five engine colors, ten algorithms (was four colors, eight algorithms)
- Version display uses `CHOROBOROS_VERSION_STRING` from CMake for consistency

### Fixed

- Purple engine slider thumb now correctly uses purple asset (was incorrectly using red)
- Slider repaint on engine change for immediate visual update

---

## [1.0.1] - Earlier

- Initial release with four engines (Green, Blue, Red, Purple)
- VST3, AU, Standalone formats
- Presets: Classic, Vintage, Rich, Psychedelic, Duck, Ouroboros
