# TODO

## High Priority - Licensing and Transparency

- [ ] Resolve licensing model conflict for distribution:
  choose one clear release posture (GPL artifacts vs EULA artifacts) and apply consistently.
- [ ] Align all public/license surfaces to the chosen model:
  `README.md`, `LICENSE`, `COPYING`, `EULA.md`, About dialog/license text, packaged docs.
- [ ] Add/refresh a machine-readable third-party notice manifest
  (framework/sdk/font + license file path + shipped/not-shipped status).
- [ ] Ensure release bundles include required notices for all shipped dependencies
  (JUCE, VST3 SDK, AAX SDK, AU SDK, fonts where applicable).
- [ ] Add explicit attribution/license file for bundled JetBrains Mono font assets if missing.
- [ ] Audit `Licenses/Third Party/` and separate:
  currently-used runtime dependencies vs archived/vendor bundle remnants.

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
