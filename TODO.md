# TODO

## High Priority - Licensing and Transparency

- [x] **Beta licensing posture (v2.05):** Official Kaizen-distributed **beta binaries** are governed by `EULA.md` (including beta scope, proprietary terms, and JUCE commercial use). **Public source** in this repo remains **GPLv3** (`LICENSE` / `COPYING`) for transparency and self-builds. Documented in `README.md` (License section), `EULA.md` (Beta program and relationship to the public source repository), `installer/resources/license.html`, and About dialog fallback in `AboutDialog.cpp`.
- [x] **Align primary license surfaces for beta:** README, EULA, macOS installer HTML, About fallback — done. `LICENSE` / `COPYING` stay as the full GPLv3 text for the **published source tree** (unchanged).
- [ ] Add/refresh a machine-readable third-party notice manifest
  (framework/sdk/font + license file path + shipped/not-shipped status).
- [ ] Ensure release bundles include required notices for all shipped dependencies
  (JUCE, VST3 SDK, AAX SDK, AU SDK, fonts where applicable).
- [ ] Add explicit attribution/license file for bundled JetBrains Mono font assets if missing.
- [ ] Audit `Licenses/Third Party/` and separate:
  currently-used runtime dependencies vs archived/vendor bundle remnants.

## Beta Feedback Sprint Plan

See `feedback/ISSUES_MASTER.md` for full issue list with reporter details and priority.

### Sprint 1: Stability (v2.04) — COMPLETE
- [x] **C1** Fix DAW freeze on plugin close — stopTimer() added to 4 Timer-inheriting destructors (DevPanel, AnimatedToggleButton, LabelWithContainer, PluginProcessor)
- [x] **H3** BBD Clock Min fix — raised from 2000→6000 Hz, filter cutoff 1200→3000 Hz (ChorusDSP.h + PluginProcessor.cpp factory defaults)
- [x] **H4** Gain staging fix — moved pre-emphasis after pushDrySamples (wet-path only), reduced bloom/focus/tape wet gains
- [x] **C2** Documented macOS 10.13 minimum — updated README.md, Release/README.md, INSTALL.txt, KNOWN_ISSUES.md
- [x] **C4** Improved install.sh — added macOS version check, 3-step quarantine removal (source→copy→verify), sudo fallback, better troubleshooting output. Updated INSTALL.txt with full Gatekeeper workaround guide.

### Sprint 2: DSP & Lifecycle (v2.05) — COMPLETE
- [x] **H1/H2** Depth zipper fix — tightened depthSmoothingMs 150→50, depthRateLimit 0.25→2.0 (ChorusDSP.h). HQ clicks fix — immediate HQ callback in parameterChanged() (PluginProcessor.cpp), dryWet.reset() + crossfade buffer clear in switchCore() (ChorusDSP.cpp)
- [x] **M3** SmoothedSlider drift fix — snapVisualToValue() on mouseUp, mouseWheelMove override with 50ms idle snap (SmoothedSlider.h/.cpp)
- [x] **M5** Dev Panel labels — renamed "Profile"→"Engine", "Core"→"Core Assignment (Advanced)", added separator label, dimmed core row styling, updated tooltips (DevPanel.cpp, DevPanelRuntime.cpp)
- [x] **M8** Factory preset protection — saveFactory() now write-once (no-op if file exists), added forceWriteFactory() for dev use, resetToFactoryDefaults() writes to user file only (DefaultsPersistence.h/.cpp, DevPanelPersistence.cpp)
- [x] **M9** Feedback text clipping — dialog 500×440→500×540, flexible text editor layout via removeFromBottom, resizable with min 500×400 (FeedbackDialog.cpp)

### Sprint 3: Refinement (v2.06) — COMPLETE
- [x] **M10** Lower Green (1.2→0.65 Hz) and Black (1.2→0.8 Hz) default Rates + matching factory presets
- [x] **M11** Reduce centreDelaySmoothingMs (150→60) and tapeDelaySmoothingMs (180→90) for snappier knob response
- [x] **H5** Document HQ/NQ spread differences (by design — different DSP topologies, not a bug) — see KNOWN_ISSUES.md

### Sprint 4: Core DSP Fixes (v2.04-dev) — COMPLETE
- [x] **BBD (Red NQ):** First-order hold interpolation eliminates S&H clock aliasing (phaser-like sweep); a1 sign fix in Butterworth cascade; raised clock/filter minimums
- [x] **Tape (Red HQ):** Damped phase integrator (DC drift fix); widened LFO smoothing bandwidth (rate knob responsiveness)
- [x] **Thiran (Blue HQ):** 32-sample linear coefficient interpolation eliminates zippering and broadband noise from per-sample DFII-T state transients
- [x] **HQ toggle:** Fast 43 ms quality switch (18 ms warmup + 25 ms crossfade); click-to-toggle UI; less sensitive drag (4→12 px threshold)
- [x] **Post-sum compressor:** Transparent peak catcher replaces legacy full-output compressor; per-core output trim during crossfade

### Blocked
- [ ] **C4** Code signing + notarization (BLOCKED — waiting on DUNS number)

## Active Product Work

- [ ] Tune all 10 engines (5 engines x 2 HQ/NQ variants) to final production targets.
- [ ] Final pass on per-engine UI visual tuning:
  slider/text/knob sizing and offsets.
- [ ] Complete remaining art pipeline tasks:
  rerender final knob/switch assets where still pending.
- [ ] Revisit asset resolution/compression strategy:
  sprite frame size, look-and-feel frame mapping, PNG optimization.
- [ ] Windows build hardening and validation pass.
- [ ] Linux build hardening and validation pass.
- [ ] Expand regression tests:
  defaults persistence, core-assignment migration/fallback, console command performance gates.

## Packaging and Release

- [ ] Verify packaging scripts end-to-end (`scripts/package.sh`, `create_dmg.sh`) with current versioning and paths.
- [ ] Add a release checklist that includes license/attribution bundle verification before tagging.

## Completed (Checked)

- [x] Dev Panel refactored from monolith into modular runtime/build/persistence/support files.
- [x] Engine-adaptive theming integrated (profile accent drives panel theme tokens).
- [x] Left-column refresh and value/readout color refresh fixed on engine/profile switch.
- [x] Validation "Recent Touches" replaced by interactive Console UI.
- [x] Added broad console command surface:
  engine/state, parameter tuning, history/undo, introspection/utilities, import/export, script helpers.
- [x] Added extended console operations:
  `add/sub`, `watch/unwatch`, `solo/unsolo`, `macro`, `toggle`, `sweep`, `alias`.
- [x] Added core/slot assignment commands:
  `core list`, `core show`, `slot show`, `slot set` with duplicate warn-but-allow behavior.
- [x] Improved console UX/perf:
  reduced long list output pain and added waiting indicator behavior.
- [x] Updated help output to include expanded command set.
- [x] Added contextual tooltip coverage across telemetry/controls (no generic duplicate tips).
- [x] Dev Panel minimum size set to `1028x525`.
- [x] Added panel edge spacing/margins to avoid controls pressed against window borders.
- [x] Renamed Validation subtab label from "Live Log" to "Console".
- [x] Standardized value text input precision handling to three decimals where applicable.
- [x] Improved rapid drag-toggle behavior for HQ/engine switching interaction.
- [x] Reworked Tone tab:
  engine-specific "Engine Response" behavior and controls.
- [x] Added non-expandable saturation controls row layout under transfer curve, theme-matched.
- [x] Exposed HPF/LPF controls in saturation response workflow.
- [x] Reworked Engine tab:
  moved engine internals controls to right-column flow area, flattened inspector mini-sections.
- [x] Added engine-specific identity/macro tab behavior alignment.
- [x] Reworked Modulation tab:
  removed unnecessary second-level tabs, unified left/right scopes + trajectory + workbench.
- [x] Added right-click layout propagation actions in Look and Feel
  (apply to all engines / apply to HQ or NQ cohort).
- [x] Added Settings tab UI preferences:
  tutorial launch, text options, theme/accessibility controls, help/feedback links.
- [x] Added interactive tutorial system and expanded tutorial flow coverage across tabs/sections.
- [x] Added section-skip/next-section tutorial navigation behavior.
- [x] Added lazy/need-to-know UI refresh strategy for heavy visual sections.
- [x] Implemented modular chorus core assignment architecture behind feature flag with legacy fallback.
- [x] Added active core selector dropdown in Dev Panel top target row with short core tokens.
- [x] Updated tutorials to match current UI behavior and tab labels.
- [x] Expanded and redesigned `docs/choroboros_docs.html` user manual content.
- [x] Applied JetBrains-based docs typography and persistent left navigation layout.
- [x] Added Kaizen DSP 2026 branding and public website links in HTML docs.
- [x] Added dedicated "Licenses and Attribution" section in HTML docs for transparency.
- [x] Added `docs/user/` and `docs/developer/` documentation structure.
- [x] Added Black engine preset to preset list.
- [x] Added engine identifiers to preset names.
