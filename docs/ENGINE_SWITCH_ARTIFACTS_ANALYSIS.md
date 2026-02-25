# Engine Switch Clicks, Pops & Zippering — Butterfly Effect Map

This document maps all identified causes of audio artifacts when switching between chorus engines (Green/Blue/Red/Purple/Black), and the possible fixes for each.

---

## 1. ROOT CAUSE: New Core Delay Line Reset → Near-Silence During Crossfade

**What happens:**  
`switchCore()` calls `newCore->reset()`, which clears the delay line to zeros. During the 15 ms crossfade, the new core outputs near-silence (reading from an empty delay). We crossfade from full chorus to near-silence → audible level drop and click.

**Evidence:**  
- `ChorusDSP::switchCore()` line 516: `newCore->reset()`
- Lagrange cores: `delayLine.reset()` → zeros
- BBD/Tape: stages filled with zeros, filter states cleared

**Fixes:**
1. **Don’t reset on switch** — Only reset cores in `prepare()` / `releaseResources()`. Reused cores keep their state; first switch to each core still has empty delay.
2. **Longer crossfade** — Increase crossfade to 50–100 ms so the new core’s delay fills during the fade and the transition is masked.
3. **Pre-fill new core** — Run the new core for one block with the current input before starting the crossfade (no reset), so its delay is partially filled.
4. **Crossfade dry instead of wet** — During switch, briefly crossfade toward dry, then back to wet as the new core warms up (more complex).

---

## 2. ROOT CAUSE: Parameter Profile Instant Jump (Rate, Depth, Color, etc.)

**What happens:**  
`applyEngineParamProfile()` sets rate, depth, offset, width, mix, color via `setValueNotifyingHost()`. These can jump (e.g. 0.5 Hz → 1.4 Hz, 50% → 35% depth). Smoothed values ramp, but large steps can still be audible.

**Evidence:**  
- `PluginProcessor::parameterChanged()` → `applyEngineParamProfile()`
- `setValueNotifyingHost()` updates parameters immediately
- `updateDSPParameters()` then calls `setRate()`, `setDepth()`, etc. with new targets

**Fixes:**
1. **Smoother parameter ramps** — Increase smoothing times for engine-switch events (e.g. 50–100 ms for rate/depth/color).
2. **Defer profile application** — Apply profile over several blocks instead of one, using internal ramping.
3. **Match profile to current engine** — When switching, optionally keep current values instead of loading profile if they’re “close enough.”
4. **Rate limit all parameters** — Extend depth-style rate limiting to rate, color, width, offset.

---

## 3. ROOT CAUSE: Depth Mapping Change (Purple 0.45×) → Centre Delay Jump

**What happens:**  
`mapDepthToEngineRange()` differs per engine (Purple uses 0.45×). On switch, `currentCentreDelayMs` is recalculated with the new mapping. Both old and new cores get this new value during crossfade, so the previous core suddenly receives a different centre delay → discontinuity.

**Evidence:**  
- `ChorusDSP::mapDepthToEngineRange()` — Purple: `normalizedDepth * 0.45f`
- `processChorusParameters()` uses `currentColorIndex` for mapping
- Crossfade uses one `currentCentreDelayMs` for both cores

**Fixes:**
1. **Per-core centre delay during crossfade** — Compute and pass separate centre delays for old and new cores based on their own mappings.
2. **Smooth centre delay on switch** — Ramp `currentCentreDelayMs` from old to new over the crossfade instead of jumping.
3. **Unify depth mapping** — Use a single mapping and let engines interpret it internally (larger refactor).

---

## 4. ROOT CAUSE: Runtime Tuning Snapshot Lag (applyRuntimeTuning on Timer)

**What happens:**  
`restoreEngineInternalsToDsp()` updates `runtimeTuning`, but `runtimeTuningSnapshot` is updated by `applyRuntimeTuning()` on a 10 Hz timer. For up to ~100 ms, the audio thread still uses the previous engine’s `centreDelayBaseMs`, `centreDelayScale`, BBD/tape params, etc.

**Evidence:**  
- `PluginProcessor::timerCallback()` at 10 Hz → `applyRuntimeTuning()`
- `restoreEngineInternalsToDsp()` only updates `runtimeTuning`
- Process uses `runtimeTuningSnapshot` (e.g. `calculateCentreDelay()`)

**Fixes:**
1. **Immediate snapshot update on engine switch** — Call `applyRuntimeTuning()` from the message thread when engine changes (e.g. in `parameterChanged` or a dedicated handler), holding `dspLock`.
2. **Higher timer rate on switch** — Temporarily increase timer frequency when engine changes.
3. **Audio-thread-safe snapshot update** — Copy only non-heap fields from `runtimeTuning` to `runtimeTuningSnapshot` on the audio thread; keep IIR/compressor updates on the message thread.

---

## 5. ROOT CAUSE: LFO Phase Offset Instant Change

**What happens:**  
Profile can change offset (e.g. 90° → 22°). `setOffset()` updates `lfoPhaseOffset` directly. Stereo phase relationship jumps → possible click.

**Evidence:**  
- `ChorusDSP::setOffset()`: `lfoPhaseOffset = offsetDegrees`
- No smoothing on phase offset
- `processChorusLFO()` uses `lfoPhaseOffset` for cos/sin

**Fixes:**
1. **Smooth phase offset** — Add a `SmoothedValue` for `lfoPhaseOffset` and ramp over 20–50 ms.
2. **Defer offset change** — Apply offset change only after crossfade completes.

---

## 6. ROOT CAUSE: Dry/Wet Mix Jump

**What happens:**  
Profile can change mix (e.g. 50% → 100%). `dryWet.setWetMixProportion(mix)` may update instantly. JUCE `DryWetMixer` may or may not smooth internally.

**Evidence:**  
- `applyEngineParamProfile()` sets mix
- `ChorusDSP::setMix()`: `dryWet.setWetMixProportion(mix)`

**Fixes:**
1. **Verify DryWetMixer smoothing** — Check JUCE implementation; if no smoothing, add a wrapper that ramps mix.
2. **Smooth mix on engine switch** — Ramp mix over 20–50 ms when engine changes.

---

## 7. ROOT CAUSE: Compressor Reacting to Level Change

**What happens:**  
Output level changes when switching cores (e.g. different saturation, different wet level). Compressor (attack 50 ms, release 200 ms) reacts to the level change → pumping or transient.

**Evidence:**  
- Compressor after chorus in `ChorusDSP::process()`
- `compressorAttackMs`, `compressorReleaseMs` in `RuntimeTuning`

**Fixes:**
1. **Bypass compressor during crossfade** — Temporarily set mix to 0 or use a separate bypass flag for the crossfade period.
2. **Slower compressor response** — Increase attack/release during switch.
3. **Level-normalize cores** — Ensure all cores have similar output levels so compressor sees less change.

---

## 8. ROOT CAUSE: Depth Rate Limiter → Slow Ramp (Zippering)

**What happens:**  
Depth is rate-limited (0.25 per second). A large profile change (e.g. 50% → 35%) ramps over ~600 ms. Each block advances the value by a small step → audible stepping (zippering).

**Evidence:**  
- `processChorusParameters()`: `depthRateLimit * (blockNumSamples / sampleRate)`
- Default `depthRateLimit = 0.25f`

**Fixes:**
1. **Relax rate limit on engine switch** — Allow a one-time faster ramp when engine changes.
2. **Use exponential smoothing only** — Remove rate limiter and rely on smoothing; may increase risk of crackling on fast manual changes.
3. **Per-sample smoothing** — Use a smoother that ramps in a fixed time (e.g. 50 ms) regardless of step size.

---

## 9. ROOT CAUSE: Different Core Output Characteristics

**What happens:**  
Cores differ in saturation, filtering, modulation. BBD vs Lagrange vs Tape vs Orbit produce different spectra and levels. Crossfade between them can expose spectral and level discontinuities.

**Evidence:**  
- Each core implements `processDelay()` differently
- BBD: input/output filters, clock
- Tape: wow/flutter, drive
- Orbit: phase warping

**Fixes:**
1. **Longer crossfade** — 30–50 ms to better mask spectral differences.
2. **Level matching** — Measure and normalize output levels per core.
3. **Spectral crossfade** — More complex; crossfade in frequency bands.

---

## 10. ROOT CAUSE: Block-Boundary Timing

**What happens:**  
Engine switch occurs at a block boundary. If the switch happens mid-block, one block can contain a mix of old and new behaviour, which can sound like a click at the boundary.

**Evidence:**  
- `updateDSPParameters()` called at start of each `processBlock()`
- Switch is effectively block-aligned

**Fixes:**
1. **Sub-block crossfade** — Already done; crossfade is sample-accurate within the block.
2. **Switch at zero-crossing** — Defer the actual switch until an approximate zero-crossing (complex, may add latency).

---

## 11. ROOT CAUSE: Saturation (Color) Jump

**What happens:**  
`mapColorToEngineRange()` and `applySaturation()` use `smoothedColor`. On switch, `setEngineColor()` calls `smoothedColor.setTargetValue(mapColorToEngineRange(color))`. If the profile’s color differs, we get a ramp. `mapColorToEngineRange` also changes with engine, so the target can jump.

**Evidence:**  
- `ChorusDSP::setEngineColor()`: `smoothedColor.setTargetValue(...)`
- `processSaturation()` uses `smoothedColor.getNextValue()`

**Fixes:**
1. **Longer color smoothing on switch** — Temporarily increase `colorSmoothingMs` when engine changes.
2. **Don’t remap color during crossfade** — Use the previous engine’s mapping for the crossfade, then switch.

---

## 12. ROOT CAUSE: Order of Operations in updateDSPParameters

**What happens:**  
`updateDSPParameters()` does: set rate/depth/offset/width/color → persist/restore internals → `setEngineColor` → `setQualityEnabled` → set mix. Restoring internals before `setEngineColor` means the new engine’s internals are applied while we may still be on the old engine for one block.

**Evidence:**  
- `PluginProcessor::updateDSPParameters()` lines 668–701
- `restoreEngineInternalsToDsp()` before `setEngineColor()`

**Fixes:**
1. **Set engine first** — Call `setEngineColor()` (and `setQualityEnabled()`) before `restoreEngineInternalsToDsp()` so the active engine matches the internals.
2. **Batch all changes** — Apply engine, params, and internals in a single atomic update to avoid inconsistent intermediate states.

---

## Summary: Priority Fixes

| Priority | Cause | Recommended Fix |
|----------|-------|-----------------|
| **P0** | New core delay reset → silence during crossfade | Don’t reset on switch, or extend crossfade to 50–100 ms |
| **P0** | Runtime tuning snapshot lag | Call `applyRuntimeTuning()` immediately on engine switch (message thread, with lock) |
| **P1** | Centre delay uses new engine’s mapping for both cores | Use per-core centre delay during crossfade, or smooth centre delay |
| **P1** | Parameter profile instant jump | Increase smoothing times on switch, or ramp profile over multiple blocks |
| **P2** | LFO phase offset jump | Smooth `lfoPhaseOffset` |
| **P2** | Depth rate limiter zippering | Relax rate limit on engine switch |
| **P2** | Compressor reaction | Bypass or soften compressor during crossfade |
| **P3** | Dry/wet mix jump | Verify and add mix smoothing if needed |
| **P3** | updateDSPParameters order | Set engine before restoring internals |

---

## Butterfly Diagram (Cause → Effect)

```
Engine Switch (User Action)
    │
    ├─► applyEngineParamProfile()
    │       ├─► Rate/Depth/Color/Offset/Width/Mix jump
    │       │       ├─► SmoothedValue ramps (zippering if steps audible)
    │       │       ├─► Depth rate limiter → slow ramp (zippering)
    │       │       └─► LFO phase offset jump → stereo discontinuity
    │       └─► dryWet mix jump → level discontinuity
    │
    ├─► restoreEngineInternalsToDsp()
    │       └─► runtimeTuning updated, but runtimeTuningSnapshot LAGS (timer)
    │               └─► Wrong centre delay, BBD/tape params for 0–100 ms
    │
    ├─► setEngineColor() → switchCore()
    │       ├─► newCore->reset() → delay line EMPTY
    │       │       └─► New core outputs near-silence during crossfade
    │       │               └─► Crossfade: full chorus → silence = CLICK/POP
    │       │
    │       ├─► currentDepthTarget = mapDepthToEngineRange(depth)
    │       │       └─► Centre delay recalculated with NEW engine mapping
    │       │               └─► Previous core gets WRONG centre delay during crossfade
    │       │
    │       └─► 15 ms crossfade
    │               ├─► Too short to mask delay-line warm-up
    │               └─► Compressor reacts to level change → pumping
    │
    └─► Different core algorithms (BBD vs Lagrange vs Tape vs Orbit)
            └─► Spectral/level mismatch at crossfade boundaries
```
