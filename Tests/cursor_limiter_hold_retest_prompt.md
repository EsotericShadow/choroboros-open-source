# Cursor: Rebuild and Retest After Limiter Hold Stage Fix

## What changed

`Source/DSP/ChorusDSP.h` — `SafetyLimiterChannel` struct now has a **hold stage** between attack and release. Previously, the limiter could begin releasing gain reduction before the delayed (lookahead) version of a hot transient had passed through the output. This caused Green_NQ to leak at -0.4 dBTP under worst-case sweep parameters.

### Specific changes

1. Added `int holdCounter = 0` member
2. `prepare()` and `reset()` both zero `holdCounter`
3. `scanBlock()`: on attack (deeper reduction needed), resets `holdCounter = delaySamples` (~221 samples at 44.1k)
4. `process()`: three-phase envelope — attack ramp → hold (decrement counter) → release (exponential decay)

No new heap allocations. No latency change. No API change.

## Build

```bash
cd /Users/main/Desktop/CHOROS_MASTER/choroboros-open-source
cmake --build build --target ChoroborosCoreTest
```

If stale cache:
```bash
cmake -B build -S .
cmake --build build --target ChoroborosCoreTest
```

## Test runs

### Run 1: Green_NQ targeted retest

```bash
./build/ChoroborosCoreTest --core Green_NQ --sweep --verbose
```

**Expected**: Sweep worst true peak should now be ≤ -0.6 dBTP (was -0.4 before the fix). Exit code 0.

### Run 2: Full regression — all cores with sweep

```bash
./build/ChoroborosCoreTest --all --sweep --verbose
```

**Expected**: All 10 cores SAFE. Exit code 0. No core should regress (the hold stage makes the limiter *more* conservative, not less).

### Run 3: Build the main plugin target to confirm it compiles

```bash
cmake --build build --target Choroboros_VST3
```

## What to look for

| Check | Pass | Fail |
|---|---|---|
| Green_NQ sweep TP | ≤ -0.5 dBTP | > -0.5 dBTP |
| All other cores sweep TP | ≤ -0.5 dBTP | > -0.5 dBTP |
| NaN/Inf any core | 0 | > 0 |
| VST3 build | exit 0 | compile error |

## If Green_NQ still fails

The hold stage should close the block-boundary release gap. If it still fails, the remaining possibility is that Lagrange3rd interpolation produces inter-sample peaks in the limiter's OUTPUT (after gain application). In that case, the fix would be to add a small margin to `computeGainReductionDb` — e.g., compute reduction as if the peak were 0.5 dB hotter than measured. Report back and we'll iterate.
