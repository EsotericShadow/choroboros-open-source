# Dev Panel V2 Execution Timeline

This document builds on `/Users/main/Desktop/choroboros/docs/DEV_PANEL_SPEC.md` and turns it into an execution plan with explicit dates, success gates, and third-party review entry points.

## Scope Added Beyond the Main Spec

- Converts the phased idea into dated milestones starting **March 2, 2026**.
- Defines acceptance gates with measurable criteria.
- Adds review prompts so DSP, UX, and QA contributors can interject with concrete evidence.
- Pairs with an interactive prototype at `/Users/main/Desktop/choroboros/docs/developer/audio_overview_prototype.html`.

## Working Assumptions

- Team keeps native JUCE rendering as the target for production implementation.
- Prototype stays web-based for fast iteration and review only.
- Runtime ownership remains in DSP/runtime tuning structs; visual layers consume snapshots.

## Timeline (Proposed)

### Phase 0: Contract Freeze (March 2-6, 2026)

Deliverables:
- Final `ControlMetadata` schema (required fields only).
- `appliesTo` encoding finalized for 5 engines x 2 quality modes.
- Canonical control registry draft covering current Dev Panel controls.

Exit criteria:
- 100% of existing controls mapped to `id`, `units`, `stage`, `visibility`, `appliesTo`.
- No unresolved owner for any control (`UI`, `persistence`, `DSP consumer` explicitly named).

### Phase 1: Information Architecture Refactor (March 9-20, 2026)

Deliverables:
- Tab shell in Dev Panel (`Overview`, `Modulation`, `Tone/Dynamics`, `Engine`, `Validation`, `Layout`).
- Engine-aware conditional visibility (`appliesTo`) for control lists.
- Core/Advanced disclosure behavior.

Exit criteria:
- Zero controls visible in the wrong engine/mode during manual matrix sweep.
- No cross-engine value leakage on engine/mode switches.

### Phase 2: Tier-1 Instrumentation (March 23-April 3, 2026)

Deliverables:
- Delay trajectory view.
- Nonlinear transfer curve view.
- Signal flow with stage badges + derived readouts.

Exit criteria:
- Changing any Tier-1 control updates at least one live visual and one derived metric.
- Red engine color=0 shows 0 saturation in both readout and transfer curve.

### Phase 3: Validation Inspector (April 6-17, 2026)

Deliverables:
- Wiring chain table: `UI raw -> mapped -> snapshot -> DSP effective`.
- Warning channels for stale snapshot and no-op binding.
- Recently touched controls filter.

Exit criteria:
- Inspector catches at least one injected stale-state test and one no-op test.
- No false positives for baseline engine presets.

### Phase 4: Hardening + Review Closeout (April 20-24, 2026)

Deliverables:
- Final acceptance test matrix across all 10 engine/mode combinations.
- Performance check for visual update rates and UI responsiveness.
- Signed review notes from DSP, UX, and QA stakeholders.

Exit criteria:
- Sign-off from at least one reviewer in each role area.
- Known critical issues tracked with owner and target date.

## Acceptance Gates (Cross-Phase)

### Wiring Integrity

- Every visible control must produce a measurable change in either:
  - a DSP effective value, or
  - a derived state metric.
- Any exceptions must be explicitly tagged as informational-only.

### Persistence Integrity

- Save/load round-trip preserves both control values and derived behavior.
- Engine/mode switching cannot apply stale internals from another profile.

### Performance Guardrails

- No locks or allocations introduced on the audio callback path.
- Visual update loop remains decoupled from the audio thread.

## Third-Party Review Prompts

Use these prompts during external review:

1. Which single visualization removes the most guesswork during tuning?
2. Are any "core" controls better moved to "advanced" for default clarity?
3. Does the validation table expose enough state to debug no-op or stale mapping issues?
4. Which engine has the weakest visual metaphor right now, and what would make it actionable?
5. Are there any derived metrics that should be removed because they look precise but are misleading?

## Suggested Review Ritual

- Weekly 45-minute review.
- Run one engine deeply per session.
- Capture:
  - confusing control names,
  - missing derived values,
  - incorrect applicability,
  - false validation warnings.

## Companion Artifacts

- Main spec: `/Users/main/Desktop/choroboros/docs/DEV_PANEL_SPEC.md`
- Prototype: `/Users/main/Desktop/choroboros/docs/developer/audio_overview_prototype.html`
