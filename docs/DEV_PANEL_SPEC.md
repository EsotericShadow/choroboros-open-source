# Choroboros Dev Panel Redesign — Technical Specification

*Synthesized from research (Gemini, Cursor, community). v1.0 — Feb 2026*

---

## 1. Problem Statement

The current Dev Panel is **editable but not interpretable**. It exposes raw DSP internals as collapsible sliders and numeric labels with no visual feedback, no derived state (actual Hz, ms, dB), and no clear grouping by DSP stage. Engine-specific controls are mixed with shared ones. Users cannot easily see what a control does or whether it affects the active engine.

---

## 2. Definition of "Correctly and Meaningfully Wired"

| Requirement | Meaning |
|-------------|---------|
| **Active control** | Every visible control affects active DSP in real time for the selected engine/mode |
| **No no-ops** | No visible control is a no-op for that engine/mode |
| **Irrelevant hidden** | Every hidden control is truly irrelevant for that engine/mode |
| **Traceability** | UI value → stored value → DSP-consumed value is traceable and numerically consistent |
| **Explicit transforms** | Raw-to-mapped transforms are visible (not implicit) |
| **Derived state** | Derived behavior is visible (e.g. "HPF: 47 Hz at 48 kHz") |
| **Deterministic persistence** | Per-engine persistence is deterministic and isolated |
| **No stale internals** | Engine/HQ switches never apply stale internals from other profiles |
| **Migration preserves semantics** | Save/load/default migration preserve the same semantic meaning |
| **Single source of truth** | Each control has a single source of truth and a single ownership path |

---

## 3. Contract Layer — Parameter Metadata Schema

Every control is defined by a strict metadata schema. **Contract-first is mandatory.**

| Attribute | Type | Description |
|-----------|------|--------------|
| `parameterId` | `juce::String` | Unique ID for APVTS lookup, serialization |
| `label` | `juce::String` | Human-readable name |
| `units` | `juce::String` | Display suffix: "Hz", "ms", "dB", "%", "Samples" |
| `range` | `juce::NormalisableRange<float>` | Min, max, default, skew/mapping |
| `appliesTo` | `juce::uint32` (bitmask) | Engines/modes where this param is active. Drives conditional visibility. |
| `dspStage` | Enum | Signal flow category: PreEmphasis, LFO, DelayLine, Filter, Output, etc. |
| `derivedHook` | `std::function` | Callback: raw 0–1 → actual derived state (e.g. cutoff Hz) |
| `uiVisibility` | Enum | "Core" (visible by default) or "Advanced" (behind disclosure) |

**APVTS integration:** Host-automatable params stay in APVTS. Non-automatable internals live as ValueTree children, sharing UndoManager and serialization without cluttering DAW automation.

**Thread safety:** Lock-free MPSC ring buffer for UI→DSP handoff. Atomic pointers for engine swaps. No mutex on audio thread.

---

## 4. Tab Structure

| Tab | Purpose | Contents |
|-----|---------|----------|
| **Overview** | Macro view | Signal flow diagram, pre/wet/post meters, key derived readouts, latency |
| **Modulation** | LFO and temporal | Phase scope, offset viz, delay trajectory per channel |
| **Tone/Dynamics** | Spectral and amplitude | Spectrum + HP/LP/pre-emphasis overlays, compressor/saturation transfer curves |
| **Engine** | Engine-specific only | Green Bloom, Blue Focus, Red BBD/Tape, Purple Warp/Orbit, Black Intensity/Ensemble — driven by `appliesTo` |
| **Layout** | UI aesthetics | Slider positions, value FX (glow, reflect, flip), scaling — separated from DSP |
| **Validation** | Wiring diagnostics | UI value → Mapped → Snapshot → DSP effective; stale/no-op warnings |

---

## 5. Visual Instrumentation (Priority Order)

### Tier 1 — Highest ROI
1. **Delay-time trajectory plot** — M[n] in ms and samples over time. Makes modulation depth/rate tangible. Would have exposed the BBD swell bug.
2. **Nonlinear transfer curve** — Input vs output amplitude for saturation/compressor. Shows where signal crosses into nonlinearity.
3. **Live signal flow diagram** — Block diagram with stage badges (on/off, gain, cutoff). Updates as params change.

### Tier 2 — High Value
4. **Spectrum analyzer + filter overlays** — HP/LP/pre-emphasis/tape/BBD curves overlaid. FFT on background thread; circular buffer from audio thread.
5. **LFO oscilloscope** — Left/right phase, offset. Verifies stereo spread.
6. **Engine-specific macro panel** — Color interpretation + derived values (e.g. Bloom → LPF cutoff, gain).

### Tier 3 — Nice to Have
7. **Stereo vectorscope / correlation meter** — Phase cancellation warning for mono sum.
8. **Per-engine visual metaphors** — Green: THD bar; Blue: phase-shift bands; Red: wow/flutter sparkline; Purple: orbit polar plot; Black: delay-tap density histogram.

---

## 6. Implementation Mechanics

### FFT / Spectrum
- **Never** run FFT on audio thread.
- Push samples into lock-free circular FIFO from `processBlock`.
- Background thread: window (Hann/Hamming), FFT, perceptual weighting (+3–4.5 dB/octave tilt), log frequency axis.
- JUCE tutorial: "Visualise the frequencies of a signal in real time". Forum: "A guideline on better Spectrum Analyzers".

### Rendering
- **Recommendation:** Native C++ with JUCE 8 Direct2D + `juce_animation`. WebView introduces IPC latency and is unsuitable for high-frequency waveform rendering.
- VBlankAnimatorUpdater for smooth transitions.
- GPU-backed rendering offloads rasterization from CPU.

### Progressive Disclosure
- **Staged:** Select filter type before revealing Q/resonance.
- **Conditional:** "Advanced" triangle reveals deep tuning. Core controls visible by default.
- **Search/filter:** By ParameterID, label, "recently touched".

---

## 7. Engine-Specific Visual Metaphors

| Engine | Characteristic | Visual |
|--------|----------------|--------|
| Green Bloom | Harmonic expansion, transient enhancement | Radial bloom or THD bar (2nd, 3rd, higher harmonics) |
| Blue Focus | Spectral tilt, phase alignment | Spectral focus bands, phase-shift graphs |
| Red BBD/Tape | Analog degradation, tape saturation | Wow/flutter sparkline, HF roll-off Bode plot |
| Purple Warp/Orbit | Phase warping, 2D modulation | Polar orbit plot (delay = radius, phase = angle) |
| Black Intensity | Multi-voice ensemble | Delay-tap density histogram |

---

## 8. Validation Tab

**Wiring Inspector** — For each (selected or recently touched) parameter:
- UI Value (0–1 normalized)
- Mapped Value (scaled, e.g. 440 Hz, 15.5 ms)
- Snapshot Value (APVTS atomic)
- DSP Effective Value (actual value in `processBlock`)

**Stale / No-Op Warnings:** Cross-reference `appliesTo` with active engine. Flag params that are changed but mathematically isolated from the signal path.

**pluginval integration:** Headless CI tests that sweep params and assert derived metrics. Tracktion pluginval for host compatibility.

---

## 9. Implementation Timeline (Phased)

### Phase 1 — Contract + Reorganization (2–3 weeks)
- Define metadata schema and register all existing controls.
- Implement `appliesTo`-driven visibility. Engine tab shows only relevant controls.
- Tab structure: Overview (placeholder), Modulation (placeholder), Tone (placeholder), Engine (current internals reorganized), Layout (extract from current), Validation (basic: UI vs snapshot).

### Phase 2 — Tier 1 Visuals (2–3 weeks)
- Delay-time trajectory plot.
- Nonlinear transfer curve (saturation/compressor).
- Live signal flow diagram (simplified blocks + badges).

### Phase 3 — Tier 2 Visuals (2–3 weeks)
- Spectrum analyzer with circular buffer + background FFT.
- Filter curve overlays (HP/LP/pre-emphasis).
- LFO oscilloscope.

### Phase 4 — Polish + Validation (1–2 weeks)
- Full Validation tab (UI → Mapped → Snapshot → DSP effective).
- Stale/no-op detection.
- Per-engine visual metaphors (Tier 3) as time permits.

---

## 10. Key References

- JUCE: "Visualise the frequencies of a signal in real time", "A guideline on better Spectrum Analyzers"
- Ross Bencina: "Real-time audio programming 101: time waits for nothing"
- FabFilter Pro-Q 3: Spectrum + EQ curve paradigm
- Tracktion pluginval: Plugin validation
- ADC: "PANEL: Tabs or Spaces? - Audio Dev Best Practices", "Using Locks in Real-Time Audio Processing, Safely"

---

## 11. Audio Overview Tab — Wireframe Concept

```
┌─────────────────────────────────────────────────────────────────────────┐
│  OVERVIEW                                    Engine: Red NQ  │  HQ: Off  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  SIGNAL FLOW                                                             │
│  ┌──────┐   ┌─────────────┐   ┌──────────┐   ┌─────────┐   ┌───────┐  │
│  │Input │──▶│ Pre-Emphasis│──▶│ HPF 47Hz │──▶│ Chorus  │──▶│ Saturation│
│  │      │   │ 3kHz +1.2dB │   │ LPF 8.5k │   │ BBD     │   │ Drive │  │
│  └──────┘   └─────────────┘   └──────────┘   └──────────┘   └───────┘  │
│       │              │               │              │             │      │
│       ▼              ▼               ▼              ▼             ▼      │
│  [ON] [ON]       [ON]            [47Hz][8.5k]  [0.62Hz][21%]  [0.5]   │
│                                                                         │
│  METERS                                                                 │
│  Pre ─████████░░░░░░░░ -6 dB    Wet ─████████████░░░░ -3 dB    Post ─██ │
│                                                                         │
│  DERIVED READOUTS                                                        │
│  Latency: 0 samples   │  Delay: 9.3 ms   │  Clock: 52.2 kHz   │  GR: -2 dB │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

**Behavior:** Stage badges update in real time as sliders change. Meters show pre/wet/post levels. Derived readouts show actual DSP state (not raw slider values).

---

## 12. Open Decisions

- [ ] Exact `appliesTo` bitmask encoding (5 engines × 2 modes = 10 bits).
- [ ] Whether Layout tab stays in Dev Panel or moves to separate preferences.
- [ ] Update rate for visualizations (e.g. 30 fps vs 60 fps).
- [ ] Whether to support WebView for rapid prototyping of Overview/Modulation tabs before native implementation.
