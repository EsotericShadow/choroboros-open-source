# HIL-B — Depth and colour zippering

## Depth: overlapping paths
1. **Raw depth** → rate limiter (`currentDepthTarget`) → exponential smoother (`smoothedDepthValue`, τ from `depthSmoothingMs`).
2. **Engine mapping** `mapDepthToEngineRange(smoothedRaw)` once per block in `processChorusParameters`.
3. **Green bloom** can scale `currentDepth` and `centreDelayMs` from `colorBlockValue` (block start unless sample path consumes colour).
4. **LFO amplitude** `oscVolume.setTargetValue(currentDepth * 0.5f)` with short linear ramp (`reset(..., 0.005)` in `ChorusDSP::reset`); `AudioBlock::multiplyBy(oscVolume)` advances per sample when smoothing.
5. **Centre delay ms** `smoothedCentreDelay` **Linear** smoother: previously `getNextValue()` once + `skip(block-1)`, which **discarded** intra-block trajectory for cores. **Now:** `centreDelayPerSampleMsBuffer[i] = getNextValue()` for each sample; delay cores read per-sample ms when the buffer is sized.

### Depth path ownership
- **Knob → APVTS →** `setDepth` only stores target; **audio thread** owns smoothers in `ChorusDSPProcess::processChorusParameters` / `processChorusLFO`.
- **Dev Panel** macros use `makeLiveMappedControl` → same APVTS IDs as the main editor.

## Colour: overlapping paths
1. **APVTS** `setColor` → `mapColorToEngineRange` → `smoothedColor` (τ from `colorSmoothingMs`, default **26 ms** in `RuntimeTuning`).
2. **Engines:** Green/Blue wet character and Red NQ saturation advance `smoothedColor` per sample where applicable; others use `skip(block)` when colour not consumed (see `processChorus`).
3. **Black NQ / HQ ensemble:** colour enters depth/tap scaling with per-sample smoothed colour inside ensemble path.

### Colour path ownership
- **Macro semantics** differ by engine; consolidation of “all colour smoothing” into one block would break engine-specific curves.

## Recommendation
- **Do not** merge depth and colour smoothers.
- **Do** keep centre-delay consumption **sample-aligned** with the `SmoothedValue` trajectory (implemented).
- Optional further work: audit **BBD/Tape** for per-sample centre if zipper reports persist there.

## Implementation status
Per-sample centre delay buffer + core reads (**Cubic, Lagrange 3/5, Linear, Linear Ensemble, Phase Warp, Orbit**). **Thiran (Blue HQ)** uses **block-constant** centre ms only — feeding per-sample centre into its inner smoother + xfade path caused non-chorus “crush”/alias-like motion in listening tests. BBD/Tape unchanged (different remapping and filter design cost).
