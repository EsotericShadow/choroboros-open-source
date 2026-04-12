# Cursor: Rebuild and Retest — Safety Limiter Fix

## What changed

The `SafetyLimiter` in `Tests/DspChainTest.cpp` was broken. It used an envelope follower with a **smoothed 0.1ms attack**, which means peaks pass through before the envelope catches up. For white noise or any signal with rapidly changing peaks, the limiter was always lagging behind.

The fix: **instant attack** (zero-time). When `|sample| > envelope`, the envelope jumps to `|sample|` immediately — no smoothing. Release is still smoothed at 50ms to prevent gain chattering.

This is the standard design for a brick-wall safety limiter. The existing peak catcher (2:1, smoothed attack) is a dynamics shaper — it's meant to sound transparent. The test harness safety limiter uses a **−1.0 dBFS threshold** (20:1, 2 dB knee, instant attack) so peaks settle **below a −0.5 dBFS** validation line under stress, with no digital overs above 0 dBFS.

## Rebuild

```bash
cmake --build build --target ChoroborosDspChainTest
```

## Rerun the same three tests

### Test A: Real audio
```bash
./build/ChoroborosDspChainTest --wav Tests/BREAK_MY_FINALITY.wav
```
Last time: +2.5 / +2.1 dBFS, limiter did nothing. This time the limiter should catch it.

### Test B: Signal sweep
```bash
./build/ChoroborosDspChainTest --sweep-signals --limiter
```
Last time: white noise +5.4, limiter brought it to +3.7. This time all signals should be ≤ -0.2 dBFS.

### Test C: All presets with limiter
```bash
./build/ChoroborosDspChainTest --all
```
Last time: everything still clipped with limiter. This time the "with limiter" table should show all SAFE.

## What to report

Paste the full output from all three runs. The key numbers I need:
1. BREAK_MY_FINALITY.wav with limiter — does it stay under 0 dBFS?
2. White noise sweep with limiter — peak level?
3. The "with limiter" comparison table — any preset still clipping?

If everything shows SAFE with the limiter, we have a validated fix ready to integrate into the production DSP chain.
