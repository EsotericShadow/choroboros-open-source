# Changelog

All notable changes to Choroboros are documented here.

## [2.04-dev] - 2026-03-14

### Fixed
- **BBD (Red NQ) phaser sweep:** S&H clock images aliased into audio band. Added first-order hold interpolation (~40 dB alias rejection). Fixed a1 coefficient sign in 5th-order Butterworth cascade. Raised bbdClockMinHz 2000→6000, bbdFilterCutoffMinHz 1200→3000.
- **Tape (Red HQ) rate knob:** Undamped phase integrator caused DC drift (~73 samples). Added damping (tapePhaseDamping 1.0→0.99999). Widened LFO smoothing bandwidth (fc 10→56 Hz) so high rates track properly.
- **Thiran (Blue HQ) zippering/noise:** Per-sample 5th-order coefficient recomputation caused DFII-T state transients. Added 32-sample linear coefficient interpolation (~30 dB transient reduction). Delay smoothing 0.998→0.9985.
- **HQ toggle latency:** Quality switch reduced from ~146 ms to ~43 ms (18 ms warmup + 25 ms crossfade). Minimum severity floor 0.40→0.10 for quality toggles.

### Changed
- **Signal chain:** Pre-emphasis moved inside processChorus (wet-path only). Legacy juce::dsp::Compressor replaced with transparent post-sum peak catcher (-2 dB / 2:1 / 4 dB knee / 1 ms attack / 100 ms release).
- **Per-core output trim:** Added virtual getOutputTrim() to ChorusCore base class; applied during crossfade blending.
- **Runtime tuning:** depthSmoothingMs 150→50, depthRateLimit 0.25→2.0, centreDelaySmoothingMs 150→60, tapeDelaySmoothingMs 180→90, tapeWetGain 1.15→1.05, greenBloomGain 0.10→0.05, blueFocusOutputGain 0.08→0.04.
- **Presets:** Green default rate 1.2→0.65 Hz, Black default rate 1.2→0.8 Hz.
- **Engine profiles:** Added migrateKnownBadEngineParamProfiles() to detect and replace stale bundled profiles on load.
- **HQ toggle UI:** Single click now toggles (was double-click only). Drag threshold 4→12 px. Slider drag sensitivity disabled to prevent accidental value changes.
- **DevPanel UX:** Renamed "Profile"→"Engine", visually separated core assignment as advanced feature, improved tooltips. Factory reset loads from bundled factory sheet instead of regenerating.

### Added
- **Session event log:** Lightweight ring buffer (last 64 events) tracks engine switches, HQ toggles, preset loads, DSP anomalies (NaN/Inf/clipping), and host info. Flushed to disk every 30 s so it survives crashes.
- **Crash reporter:** Platform-specific signal handlers (macOS: SIGSEGV/SIGABRT/SIGFPE/SIGBUS/SIGILL; Windows: SetUnhandledExceptionFilter) flush the session log on abnormal exit. On next launch, if a clean-shutdown marker is missing, the user is prompted to send the crash report.
- **Send to Developer (in-app):** Feedback dialog now has a "Send to Developer" button that opens the user's mail client pre-filled with feedback, usage summary, session log, and system info addressed to info@kaizenstrategic.ai.
- **DSP anomaly detection:** processBlock checks output for NaN, Inf, and sustained hard clipping (>2.0); logs to session log (throttled to 1 event per 2 s).
- **Host info in feedback:** Usage summary now includes DAW name, format (VST3/AU/AAX), sample rate, buffer size, CPU, and RAM.

---

## [2.03] - 2026-03-06

### Added
- **Windows support:** First official Windows release — x64 and x86-compat VST3 + Standalone builds.
- **Platform-specific factory defaults:** Mac and Windows each use bundled factory JSON; embedded fallback when config files absent.
- **Unlock safety warning:** Modal with "Hide this message in the future" and Settings > Safety toggle.
- **Release packaging:** Windows x64/x86 zip packaging script.

### Changed
- **Dev Panel:** Modularized; engine-adaptive theming; lazy tab build; command console replaced Validation tab.
- **Performance:** Deferred Dev Panel creation; theme decode overlapped with host paint; shared theme cache.
- **Editor scale:** Main editor scaled to 91%.

### Fixed
- **Windows:** MSVC lambda captures, BinaryData/getNamedResource, knob anti-aliasing, editor layout/theme APIs.
- **Windows:** Load trace script PowerShell 5.1 compatibility; single-sample timing; percentile helper.

---

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
