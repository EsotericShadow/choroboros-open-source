# Cursor: Build and Validate Production Safety Limiter Integration

## What changed

The lookahead true-peak safety limiter has been integrated into the **production DSP chain** — it now runs inside the real plugin, not just the test harness.

### Files modified

1. **`Source/DSP/ChorusDSP.h`**
   - Added `SafetyLimiterChannel` struct (nested inside ChorusDSP) — same architecture as the test harness version: 5ms lookahead delay, 4× cubic Hermite true-peak detection, soft-knee GR, smooth gain ramp, exponential release.
   - Added `safetyLimiterL` and `safetyLimiterR` members (one per channel).
   - Added `getSafetyLimiterLatencySamples()` public accessor.
   - DSP quirks audit fixes: `delaySamples` initialized to 1 (not 0) to prevent `% 0` UB; `detectTruePeak()` guards `numSamples < 4`; `process()` guards empty delay buffer.

2. **`Source/DSP/ChorusDSP.cpp`**
   - `prepare()`: calls `safetyLimiterL.prepare()` and `safetyLimiterR.prepare()` to pre-allocate delay buffers.
   - `reset()`: calls `safetyLimiterL.reset()` and `safetyLimiterR.reset()`.
   - `process()`: after `processWidth()`, scans both channels for true peaks via `scanBlock()`, then runs per-sample `process()` through the delay + gain ramp.

3. **`Source/Plugin/PluginProcessor.h`**
   - (No extra declaration.) Latency is reported only via `setLatencySamples()` in `prepareToPlay()`.

4. **`Source/Plugin/PluginProcessor.cpp`**
   - `prepareToPlay()`: calls `setLatencySamples (chorusDSP->getSafetyLimiterLatencySamples())` **after** `chorusDSP->prepare()`. JUCE’s `AudioProcessor::getLatencySamples()` is non-virtual and simply returns that stored value — do **not** add a derived `getLatencySamples() override` (it will not compile on JUCE 8).

## Build

```bash
cmake --build build
```

Build ALL targets (the full plugin, not just the test harness). If you get errors:
- Check `#include <cmath>` and `<algorithm>` are reachable from ChorusDSP.h (they should be via juce_dsp)
- The `SafetyLimiterChannel` struct uses `std::vector<float>`, `std::ceil`, `std::exp`, `std::log10`, `std::pow`, `std::abs`, `std::max` — all standard.

## Validate

### Step 1: Compile check
```bash
cmake --build build 2>&1 | tail -20
```
Must compile clean. Zero warnings from the limiter code.

### Step 2: Test harness still works (regression)
```bash
cmake --build build --target ChoroborosDspChainTest
./build/ChoroborosDspChainTest --full-matrix
```
The test harness has its own copy of the limiter. Confirm it still passes: all 10 engines SAFE with limiter, peaks ≤ -0.5 dBFS.

### Step 3: pluginval (if available)
```bash
pluginval --validate build/Choroboros_artefacts/VST3/Choroboros.vst3 --strictness-level 5
```
Check for:
- No crashes
- `getLatencySamples()` returns 221 (@ 44.1kHz) or 240 (@ 48kHz)
- No audio-thread allocation warnings

### Step 4: DAW smoke test
Load the plugin in a DAW. Set Psychedelic preset. Play broadband audio (drums, full mix). Check:
- Output meter never exceeds -0.5 dBFS
- No audible artifacts (clicks, zipper noise, distortion)
- DAW reports correct latency compensation (5ms)
- Bypass comparison: limiter should be transparent on normal presets (no audible difference unless you push width to max)

## What to report

1. Clean compile? Any warnings?
2. Test harness regression: still all SAFE?
3. pluginval results (if run)
4. DAW latency reporting correct?
5. Any audible artifacts on Psychedelic preset with loud material?

## Architecture reference

Signal chain order in `ChorusDSP::process()`:
```
Input → HPF → PreSat → Chorus Core (DW Mix + PeakCatch + Trim) → LPF → Width (M/S) → Safety Limiter → Output
                                                                                        ↑ NEW (5ms lookahead)
```

The limiter is the LAST stage. It catches everything the width stage creates, and nothing downstream can undo its ceiling.
