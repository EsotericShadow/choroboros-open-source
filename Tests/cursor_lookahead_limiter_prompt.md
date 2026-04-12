# Cursor: Rebuild and Retest with Lookahead True-Peak Limiter

## What changed

The `SafetyLimiter` struct in `Tests/DspChainTest.cpp` has been completely rewritten from an instant-attack envelope follower to a **production-grade lookahead true-peak limiter**. This is the design that will be ported directly to the production DSP chain.

### Old design (instant attack — REPLACED)
- Instant attack created discontinuous gain changes → aliasing artifacts
- At 4-6 dB GR, audible intermodulation distortion on transients
- No inter-sample peak detection

### New design (lookahead + true-peak)
- **5 ms lookahead**: Audio is delayed through a circular buffer. The limiter scans the undelayed signal for peaks, then applies a smooth gain ramp over the lookahead window so reduction is fully applied before the peak exits the delay line. No discontinuous gain changes = no aliasing.
- **4× true-peak detection**: Cubic Hermite interpolation between samples catches inter-sample peaks that the DAC would reconstruct but that sample-level measurement misses. Per ITU-R BS.1770.
- **Soft-knee GR**: Same curve as before (2 dB knee centered at -1.0 dBTP, 20:1 ratio).
- **Smoothed release**: 50 ms exponential release prevents pumping.
- **Latency**: 221 samples @ 44.1 kHz (5 ms). Will be reported to DAW via `getLatencySamples()` in production.

### Architecture
```
Per block:
  1. scanBlock() — 4× oversampled true-peak scan of raw audio
                   → sets targetGainDb and rampPerSample

Per sample:
  2. process()  — writes sample to delay buffer
                 — reads delayed sample (5 ms behind)
                 — ramps currentGainDb toward target (attack)
                   or exponentially releases (release)
                 — applies gain to delayed sample
```

## Rebuild

```bash
cmake --build build --target ChoroborosDspChainTest
```

If you get compile errors, check:
- `std::vector` is used for `delayBuf` (needs `<vector>` — already included)
- `std::ceil` needs `<cmath>` — already included
- No JUCE DelayLine dependency — the limiter uses its own circular buffer

## Run these tests and paste FULL output

### Test A: Full matrix — does the lookahead limiter handle all 10 engines?
```bash
./build/ChoroborosDspChainTest --full-matrix
```
Key: all 10 engines should show SAFE with limiter. Peaks should land around -0.7 to -1.0 dBFS (slightly lower than the old instant-attack design because true-peak detection catches inter-sample peaks the old limiter missed).

### Test B: Parameter sweep with limiter — any cells still flagged?
```bash
./build/ChoroborosDspChainTest --param-sweep --limiter
```
Key: no `!` flags in any cell. All 300 configurations ≤ -0.5 dBFS.

### Test C: Full matrix with WAV
```bash
./build/ChoroborosDspChainTest --full-matrix --wav Tests/BREAK_MY_FINALITY.wav
```
Key: all engines SAFE with limiter on real audio.

### Test D: Single Psychedelic preset — detailed view
```bash
./build/ChoroborosDspChainTest --preset Psychedelic --signal white --limiter
```
Key: shows the full topology diagram with the new lookahead limiter specs, plus detailed tap measurements.

## What to report

1. Did it compile clean?
2. Full matrix without/with limiter tables + delta table
3. Param sweep with limiter — any `!` flags?
4. WAV matrix — all SAFE?
5. Single Psychedelic output — what are the final peak levels?

## Why this matters

This limiter design is going directly into the production `ChorusDSP::process()`. The beta DSP is the commercial DSP. It needs to be correct, alias-free, and standards-compliant (EBU R128, ITU-R BS.1770). The old instant-attack design worked numerically but created audible artifacts at high GR. This one doesn't.
