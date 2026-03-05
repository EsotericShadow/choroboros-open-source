# Agent Context: Windows Transition

## Mission

Get reliable Windows builds and validation loops with minimum surprise and zero DSP regressions.

## Primary Objective

Establish a repeatable x64 pipeline that:

1. Configures cleanly on typical Windows dev machines.
2. Builds VST3 + Standalone consistently.
3. Runs regression harness in CI-safe mode where GUI is unavailable.
4. Produces artifacts in known locations and can be installed quickly for DAW testing.

## Constraints

- Do not change DSP algorithm behavior in this workspace unless explicitly requested.
- Do not change host automation parameter IDs.
- Keep changes surgical and reversible.
- Prefer additive scripts/docs over risky build-system rewrites.

## Known Risk Areas

- JUCE dependency fetch can fail on restricted networks.
- AAX and AU format behavior differs by platform/toolchain.
- Headless CI cannot reliably run GUI-heavy test flows.
- Windows path length and shell environment can break builds unexpectedly.

## Safe Defaults

- Use Visual Studio 2022 generator.
- Build x64 first; x86 is optional and legacy-oriented.
- Use `--dsp-only` regression mode in CI/headless environments.
- Install VST3 to user plugin path before DAW scan.

## Red Flags

- Build script touches DSP source unexpectedly.
- Preset/state defaults differ between macOS and Windows output.
- Regression test failures that only appear after build-system edits.
- License/notice files missing from package outputs.

## Handoff Expectations

When finishing a Windows prep or troubleshooting pass, always leave:

- exact commands executed
- pass/fail outcomes
- artifact paths
- unresolved blockers
- next actionable step
