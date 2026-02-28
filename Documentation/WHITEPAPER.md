# Choroboros: Vision & Technical Whitepaper

**A chorus that eats its own tail — Five colors, ten algorithms**

*Kaizen Strategic AI Inc. (DBA: Kaizen DSP)*  
*British Columbia, Canada*  
*Version 2.02-beta | 2026*

---

## Executive Summary

Choroboros is a commercial, multi-engine chorus plugin that reimagines modulation effects through a unified architecture of five distinct sonic characters and ten interpolation algorithms. Built on the principle that chorus is not a single effect but a family of related phenomena—from vintage BBD warmth to experimental 2D phase warping—Choroboros offers producers a single instrument that spans the full spectrum of chorus expression.

This document outlines the vision, technical architecture, design philosophy, and roadmap for Choroboros and the broader Kaizen DSP ecosystem.

---

## 1. Vision & Mission

### Vision

**To create the definitive chorus plugin—one that honors the history of the effect while extending its possibilities into uncharted territory.**

Chorus has been a staple of production since the 1970s. From the Roland CE-1 and Dimension D to modern digital implementations, each era has contributed a distinct character. Yet most plugins force a choice: vintage or modern, clean or colored, safe or experimental. Choroboros rejects that tradeoff. By architecting five independent engines—each with two algorithm variants—we provide a single plugin that can move seamlessly from classic studio warmth to psychedelic 2D modulation without compromise.

### Mission

- **Depth over breadth:** Ten carefully designed algorithms, not hundreds of presets masquerading as engines
- **Transparency and character:** Each engine has a clear sonic identity; users choose by ear, not by menu
- **Community-driven development:** Source available on GitHub during beta; contributors help shape the product before commercial release
- **Pro-grade quality:** Sample-accurate modulation, zero heap allocation in the audio path, support up to 192 kHz

---

## 2. The Ouroboros Metaphor

The name *Choroboros* fuses *chorus* with *ouroboros*—the ancient symbol of a serpent eating its own tail, representing cycles, renewal, and self-reference.

In the context of the plugin:

- **Cycles:** Chorus is inherently cyclical—an LFO modulates delay time, creating periodic pitch and phase variation. The effect feeds back on itself conceptually: the delayed signal is the source of the modulation’s perception.
- **Renewal:** Each engine “renews” the chorus concept through a different lens—classic, modern, vintage, experimental, linear. The same parameters (rate, depth, width) produce radically different results.
- **Self-reference:** The Orbit algorithm (Purple HQ) introduces 2D modulation—a chorus that modulates in a plane rather than a line. The effect becomes self-referential: modulation of modulation.

The ouroboros also reflects our development philosophy: we iterate, refine, and improve continuously. The plugin is never “finished”; it evolves.

---

## 3. Product Philosophy

### 3.1 One Plugin, Many Personalities

Choroboros is built on the belief that **chorus is a category, not a single effect**. Different interpolation methods, saturation characteristics, and modulation shapes produce fundamentally different sounds. Rather than one algorithm with many knobs, we provide multiple algorithms with a consistent control surface.

Users select an engine (color) and quality mode (Normal/HQ). The same six parameters—Rate, Depth, Offset, Width, Color, Mix—map to engine-specific behavior. This keeps the interface simple while maximizing sonic range.

### 3.2 Character Through Algorithm Design

Each engine’s character emerges from its DSP, not from post-processing:

- **Green (Classic):** Lagrange interpolation—smooth, musical, the “default” chorus sound
- **Blue (Modern):** Cubic and Thiran allpass—clean, transparent, modern production
- **Red (Vintage):** BBD and tape emulation—warmth, saturation, analog feel
- **Purple (Experimental):** Phase-warped and Orbit—non-linear, psychedelic, 2D modulation
- **Black (Linear):** Linear interpolation—transparent, CPU-efficient, utility

We avoid “character” as a separate knob. The engine *is* the character.

### 3.3 No Compromise on Core Parameters

Every engine supports:

- **Rate:** 0.01–20 Hz (sub-audio to fast vibrato)
- **Depth:** 0–100% (engine-scaled for appropriate range)
- **Offset:** 0–180° (stereo phase relationship)
- **Width:** 0–200% (mono to wide stereo)
- **Color:** Engine-specific (tone, saturation, or timbre)
- **Mix:** 0–100% dry/wet

Parameters are smoothed to prevent zipper noise and read-pointer discontinuities. The DSP is designed for real-time use with no allocations in the process path.

---

## 4. Technical Architecture

### 4.1 High-Level Structure

```
ChoroborosAudioProcessor (Plugin API)
        │
        ▼
   ChorusDSP (Controller)
        │
        ├── LFO generation (sine + cosine for stereo)
        ├── Parameter smoothing
        ├── Pre-emphasis / HPF / compressor
        ├── Width processing (mid-side)
        └── ChorusCore (swappable)
                │
                ├── ChorusCoreLagrange3rd / ChorusCoreLagrange5th   (Green)
                ├── ChorusCoreCubic / ChorusCoreThiran             (Blue)
                ├── ChorusCoreBBD / ChorusCoreTape                (Red)
                ├── ChorusCorePhaseWarped / ChorusCoreOrbit        (Purple)
                └── ChorusCoreLinear / ChorusCoreLinearEnsemble    (Black)
```

### 4.2 ChorusCore Interface

Each algorithm implements the `ChorusCore` interface:

- `prepare(spec)` — Allocate delay lines, set sample rate
- `reset()` — Clear state
- `processDelay(dsp, block, centreDelayMs)` — Process one block
- `getGuardSamples()` — Interpolation guard band
- `getMaxDelaySamples()` — Delay line bounds

The main `ChorusDSP` owns LFO generation, parameter smoothing, and shared buffers. Cores receive the modulated delay time and perform interpolation. Engine switching uses a crossfade to avoid clicks.

### 4.3 Runtime Tuning

A `RuntimeTuning` structure exposes internal parameters (smoothing times, BBD clock range, tape wow/flutter, etc.) for advanced users and developers. This enables:

- Per-engine fine-tuning without code changes
- Research and experimentation
- Future automation or preset morphing

### 4.4 Platform Support

- **Formats:** VST3, AU, AAX, Standalone
- **Platforms:** macOS (Intel + Apple Silicon universal binary)
- **Framework:** JUCE 8.x
- **Development:** Source on [GitHub](https://github.com/EsotericShadow/choroboros-open-source) during beta for community contribution; commercial release will be closed source

---

## 5. The Five Engines & Ten Algorithms

| Engine | Normal (NQ) | HQ |
|--------|-------------|-----|
| **Green (Classic)** | 3rd-order Lagrange | 5th-order Lagrange |
| **Blue (Modern)** | Cubic interpolation | Thiran allpass |
| **Red (Vintage)** | BBD emulation | Tape-style chorus |
| **Purple (Experimental)** | Phase-warped | Orbit (2D modulation) |
| **Black (Linear)** | Linear interpolation | Linear Ensemble |

### 5.1 Green — Classic

Smooth, musical, the “reference” chorus. Lagrange interpolation provides high-quality resampling with minimal artifacts. HQ increases polynomial order for even smoother modulation.

### 5.2 Blue — Modern

Clean and transparent. Cubic interpolation is efficient and neutral; Thiran allpass adds phase characteristics that suit modern production. Ideal for subtle widening and contemporary mixes.

### 5.3 Red — Vintage

Analog character through emulation. BBD (bucket-brigade delay) models the classic chorus pedals; tape adds wow, flutter, and saturation. For warmth, grit, and “that” sound.

### 5.4 Purple — Experimental

Non-standard modulation. Phase-warped chorus uses non-uniform phase; Orbit introduces 2D rotating modulation for psychedelic, evolving textures. For sound design and creative effects.

### 5.5 Black — Linear

Transparent and CPU-efficient. Linear interpolation is the baseline; Linear Ensemble adds multi-voice layering. For when you need chorus without character—or maximum performance.

---

## 6. Design Principles

### 6.1 No Batch Processing

Parameters are processed in real time. No offline rendering assumptions; the plugin behaves identically in a DAW and in standalone mode.

### 6.2 Categorical Independence

Each engine is tuned independently. We avoid “one size fits all” parameter mappings. Red’s Color controls saturation differently than Blue’s; Purple’s Rate range may differ from Green’s. Respect for nuance over uniformity.

### 6.3 Data Integrity

We do not assume data contents. Level detection, smoothing, and modulation are designed to handle edge cases (silence, clipping, DC). The DSP is defensive.

### 6.4 Admit Uncertainty

When behavior is ambiguous or implementation is incomplete, we document it. No invented results; no false confidence.

---

## 7. Development Model & Community

### 7.1 Source-Available During Beta

During the beta phase, Choroboros source code is available on GitHub. This is not permanent open source—it is a development model that enables:

- **Community contribution:** Bug reports, algorithm improvements, documentation, presets
- **Transparency during development:** Beta testers and contributors can see how the plugin works
- **Iteration before commercial release:** Feedback and patches help shape the final product

When the commercial version ships, the GitHub repository will be archived and no longer maintained. The commercial product will be closed source.

### 7.2 JUCE Licensing

Choroboros uses JUCE, which is dual-licensed (AGPLv3 or commercial). Commercial distribution will use JUCE's commercial license. See [juce.com/legal](https://juce.com/legal) for details.

### 7.3 Contributing During Beta

We welcome contributions while the repo is active: bug reports, algorithm improvements, documentation, presets. The beta feedback system (usage stats, save-to-file, email) helps us prioritize. The roadmap is public; the TODO is in the repo.

---

## 8. Roadmap & Future Vision

### 8.1 Near-Term (v2.x)

- **Engine tuning:** Finalize all 10 algorithms for production
- **UI polish:** Per-engine asset sets (knobs, backpanels) matching Green’s quality
- **Asset optimization:** Resolution, compression, frame sizing
- **Packaging:** Windows and Linux builds
- **Documentation:** User and developer guides

### 8.2 Medium-Term

- **Preset system:** User presets, import/export, factory expansion
- **MIDI learn:** Parameter mapping for hardware control
- **ARA support:** Integration with DAW timeline (where applicable)
- **Expanded regression tests:** Defaults persistence, engine coverage

### 8.3 Long-Term Vision

Choroboros is the first product in the Kaizen DSP ecosystem. The architecture—multi-engine, swappable cores, runtime tuning—is a template for future effects:

- **Modulation family:** Flanger, phaser, vibrato built on similar principles
- **Delay family:** Multi-engine delay with distinct characters
- **Reverb:** Algorithmic and hybrid approaches

The goal is a coherent suite of commercial, pro-grade effects that share design philosophy and technical rigor.

---

## 9. Conclusion

Choroboros is more than a chorus plugin. It is a statement: that effects can be deep and focused, and that the best tools emerge from clear vision and iterative refinement—with community input during development.

*A chorus that eats its own tail*—cyclical, self-referential, and endlessly renewable. We invite you to use it during beta, contribute if you wish, and help shape the commercial release.

---

*Kaizen Strategic AI Inc. | Kaizen DSP*  
*British Columbia, Canada*  
*greenalderson@gmail.com*
