# Choroboros: The Official User Manual

*Version: 2.05*
*Audience: Musicians, Mixing Engineers, and Sound Designers.*

Welcome to the official manual for **Choroboros**. 

Choroboros is far more than a standard digital chorus effect. It is a multi-modal, deep-architectural modulation suite. By wrapping ten entirely distinct mathematical chorus topologies into five color-coded "Engines," Choroboros allows you to traverse decades of audio hardware history—from the dark, noisy Bucket-Brigade delay chips of the 1970s to pristine, modern, 5th-order Lagrange-interpolated digital algorithms.

Despite the monumental complexity under the hood, the front-facing user interface is aggressively minimalist. You only need to touch six knobs to steer the beast.

This manual explains exactly what those six knobs do, what those five colors represent, and how to harness the full potential of Choroboros.

---

## Table of Contents

1.  **Installation & First Launch**
2.  **The Interface Philosophy**
3.  **The Six Master Knobs**
    *   3.1 Rate (LFO Speed)
    *   3.2 Depth (Pitch & Delay Throw)
    *   3.3 Offset (LFO Phase Offset)
    *   3.4 Width (Stereo Phase & Correlation)
    *   3.5 Color (Engine-Specific Character)
    *   3.6 Mix (Parallel Integration)
4.  **The Engine Core Selector (The Five Colors)**
    *   4.1 The Green Engine: Classic Bloom
    *   4.2 The Blue Engine: Modern & Focused
    *   4.3 The Red Engine: Vintage Analog & Tape
    *   4.4 The Purple Engine: Experimental & Asynchronous
    *   4.5 The Black Engine: Hard Linear & Ensemble
5.  **The HQ Switch (High-Quality)**
6.  **Presets & Initialization**
7.  **The Hidden "DEV" Panel**
8.  **Glossary**

---

## 1. Installation & First Launch

Choroboros comes compiled as VST3, AU, and Standalone for macOS, and VST3 and Standalone for Windows.

1.  Place the provided plugins in your operating system's default plugin directories (e.g., `/Library/Audio/Plug-Ins/VST3` on Mac, or `C:\Program Files\Common Files\VST3` on Windows).
2.  Launch your DAW (Ableton Live, Logic Pro, FL Studio, REAPER, etc.) and perform a plugin scan.
3.  Load Choroboros onto an audio track or software synthesizer.

Upon first launch, you will see a sleek, dark interface with six primary controls. The engine selector is in the top-left header area, and a small animated drawer in the top-right gives access to the Dev Panel, About, Help, and Feedback icons. By default, the engine is set to Green.

---

## 2. The Interface Philosophy

Most modern plugins paralyze users with choice. They display dozens of sliders for EQ, compression, LFO shapes, and delay times. 

Choroboros hides the math. The primary user interface is built for speed, musicality, and intuition. 
*   If you want the chorus to sound "darker," you don't need to tune a Low-Pass Biquad Filter to 800Hz with a Q of 0.7. You simply turn the **Color** knob down. 
*   If you want the chorus to move "faster," you don't need to specify 4.2Hz. You turn the **Rate** knob right.

Choroboros translates your physical macro gestures into complex, multi-variable changes behind the scenes. The UI is designed so that it is functionally impossible to make Choroboros sound "bad." Every constraint on the front panel has been meticulously tuned by DSP engineers to maintain sweet spots.

---

## 3. The Six Master Knobs

These six controls dictate the behavior of the active audio algorithm. Note that because Choroboros contains 10 different internal architectures, turning the "Rate" knob on the Green engine might physically map to a different mathematical maximum than the "Rate" knob on the Blue engine. The controls are inherently adaptive.

### 3.1 Rate 
The **Rate** knob controls the speed of the internal Low Frequency Oscillator (LFO). This invisible sweeping wave is what actively changes the delay time of the chorus, causing the signature pitch-wobble.
*   **0% (Bottom Left):** The LFO grinds to a near halt. Expect glacial sweeps that take 10 or 20 seconds to complete a single cycle. Perfect for ambient pads or slow cinematic tension.
*   **50% (Straight Up):** Standard musical chorus rates, providing a gentle undulating rhythm to guitars or vocals.
*   **100% (Bottom Right):** The LFO spins at rapid, punishing speeds. This induces heavy vibrato effects or aggressive, rotary-speaker-style thrashing. 

### 3.2 Depth
The **Depth** knob controls the *amplitude* of the LFO. It determines exactly how far the delay line is stretched and compressed.
*   **0%:** The LFO has no power. The chorus becomes a static, motionless delay line. (Useful for basic comb-filtering).
*   **Low Settings:** Subtle pitch thickening. The sound slowly detunes and re-tunes against the dry signal.
*   **High Settings:** Massive, swooping pitch dives. At extreme depths, Choroboros can output entirely dissonant microtonal pitch bends.
*   *Interaction Note:* Depth and Rate are inextricably linked. A fast Rate with a high Depth will cause extreme, seasick pitch modulation. If you push the Rate high, usually you must pull the Depth low to maintain musicality.

### 3.3 Offset
The **Offset** knob controls the LFO phase offset between the left and right stereo channels (0–180°). It determines how "out of step" the left and right modulation sweeps are.
*   **0° (Full Left):** Both channels sweep in perfect unison. The modulation is identical in left and right, producing a narrower, more centered chorus effect.
*   **90° (Default, Straight Up):** The left and right LFOs are offset by a quarter cycle. This is the sweet spot for natural-sounding stereo chorus — enough separation to feel wide without sounding disconnected.
*   **180° (Full Right):** The channels sweep in complete opposition. When the left pitch rises, the right pitch falls. This produces the widest possible stereo image from the LFO offset alone.
*   *Interaction Note:* Offset and Width both affect stereo spread but in different ways. Offset shifts the LFO *phase relationship* between channels; Width scales the overall stereo *amplitude*. Use them together for maximum spatial control.

### 3.4 Width
The **Width** knob controls the stereo spread of the chorus effect (0–200%).
*   **0% (Mono):** The Left and Right channels output identical chorus signals. The effect sits dead center in the stereo field.
*   **100% (Default):** Standard stereo width. The left and right channels have natural separation — wide enough to feel spacious without being exaggerated.
*   **200% (Maximum):** Extreme stereo spread. The channels are pushed as far apart as possible, creating a hyper-wide stereo image that wraps around the listener. Great for ambient pads and special effects, but use with caution on lead instruments — it can thin the center image.

### 3.5 Color
The **Color** knob is an engine-specific character control (0–100%). Unlike the other knobs, Color does something fundamentally different depending on which engine is active:

*   **Green — Bloom:** Adds thickness and gentle vintage softening to the wet signal. No saturation. Low values stay clean and airy; high values add density and damping.
*   **Blue — Focus:** Adds clarity and presence. No saturation. Low values are softer and wider; high values are tighter, brighter, and more articulate.
*   **Red NQ — Saturation:** Controls the drive amount on the wet path. Adds analog-style harmonic distortion to the BBD chorus output.
*   **Red HQ — Tape Tone:** Controls the tape character — tone and drive. Higher values push more tape-style coloring.
*   **Purple — Warp:** Controls how much the phase or orbit modulation shape is applied. Subtly reshapes the modulation character.
*   **Black — Mod Intensity:** Controls the modulation intensity and ensemble spread.

Because Color is adaptive, turning it up on Green produces a warm, thick bloom, while turning it up on Red pushes saturation. Think of it as the engine's own personality dial.

### 3.6 Mix
The **Mix** knob dictates the parallel blend of the unprocessed (Dry) signal against the chorused (Wet) signal.
*   **0%:** 100% Dry. The plugin outputs identical audio to the input. (Note: The internal DSP actually shuts down at 0% to save valuable CPU overhead).
*   **50%:** Equal parts Dry and Wet. The thickest, most traditional chorus sound, allowing maximum comb-filtering phase interaction.
*   **100%:** 100% Wet. True vibrato. Because there is no Dry signal to reference against, you do not hear thick chorusing; you only hear the pure pitch-bending wobble of the delayed signal.

---

## 4. The Engine Core Selector (The Five Colors)

The **Engine Core Selector** is the dropdown in the top-left header area. This is the heart of Choroboros.

Changing the Engine doesn't just change parameters; it completely unloads the current C++ algorithm from memory and hot-swaps in a totally different mathematical topology. Choroboros is effectively five plugins in one.

### 4.1 The Green Engine (Classic Bloom)
*   **Vibe:** Natural, warm, musical.
*   **Implementation:** The Green engine is built on pristine 3rd-order Lagrange interpolation for smooth, artifact-free sweeping.
*   **Secret Weapon (Bloom):** Green's Color knob applies a bloom effect — thickness and gentle vintage softening on the wet signal. It gives the chorus a lush, padded quality without adding any saturation or distortion. Perfect for acoustic instruments, pads, and vocals where you want warmth without grit.

### 4.2 The Blue Engine (Modern Focus)
*   **Vibe:** Surgical, EDM, crystalline, precise.
*   **Implementation:** Blue NQ uses cubic interpolation for clean, transparent chorus. Blue HQ uses 5th-order Thiran allpass interpolation for phase-accurate stereo imaging.
*   **Secret Weapon (Focus EQ):** Blue is designed to cut through dense electronic mixes. Its "Color" knob applies focus filters and a presence peak to the wet signal, adding clarity and articulation. Higher values make the chorus sit surgically on top of synth leads and electronic textures.

### 4.3 The Red Engine (Vintage Analog)
*   **Vibe:** Lo-Fi, noisy, warm, degraded hardware.
*   **Implementation:** Red completely abandons digital cleanliness. It physically emulates the math of failing electronic components.
*   **Secret Weapon (Analog Emulation):** 
    *   *(Normal Mode):* Emulates an old Bucket-Brigade Device (BBD). You will hear natural analog soft-clipping saturation and noticeable high-frequency roll-off (to hide the simulated clock-whine of the delay chip).
    *   *(HQ Mode):* Emulates a mechanical magnetic Tape loop. You will hear actual chaotic pitch-drifts caused by irregular tape friction (Wow and Flutter).

### 4.4 The Purple Engine (Experimental Phase)
*   **Vibe:** Ambient, unpredictable, cinematic.
*   **Implementation:** Normal choruses use a single LFO. Purple uses *two* LFOs running simultaneously at asynchronous mathematical ratios.
*   **Secret Weapon (Bi-Modulation):** Because the two LFOs multiply against each other, the resulting modulation wave is incredibly complex and almost never repeats sequentially. It prevents the human brain from identifying a "pattern," resulting in organic, seemingly randomized chorus movement perfect for 10-minute ambient drone spaces.

### 4.5 The Black Engine (Hard Linear)
*   **Vibe:** 1990s Nu-Metal, harsh digital, comb-filtered.
*   **Implementation:** Black runs low-fidelity, basic Linear interpolation math. Rather than trying to be smooth, it leans into harsh digital aliasing and edge-artifacts.
*   **Secret Weapon (Dual-Tap Ensemble):** In HQ mode, Black uses two independent delay lines (a dual-tap ensemble) with separate stereo decorrelation. This creates dense, metallic phase cancellation. It is designed specifically to make heavy distortion guitars sound impossibly massive.

---

## 5. The HQ Switch (High-Quality)

Next to the main Engine drop-down is the `HQ` toggle. 

In standard plugins, an "HQ" or "Oversampling" button just doubles the internal sample rate to prevent aliasing. In Choroboros, the HQ toggle is a literal structural fork in the road. It usually loads a completely different execution algorithm.

*   **Green Normal:** 3rd Order Lagrange.
*   **Green HQ:** 5th Order Lagrange (Significantly higher CPU, zero high-frequency artifacts).
*   **Blue Normal:** Cubic Interpolation.
*   **Blue HQ:** Thiran All-Pass Interpolation (Requires immense computational DSP but protects low-end phase perfection).
*   **Red Normal:** Bucket Brigade Device matrix.
*   **Red HQ:** Magnetic Tape Wow/Flutter matrix.
*   **Purple Normal:** Phase-Warp polynomial math.
*   **Purple HQ:** Orbital X/Y polar coordinate trigonometric math.
*   **Black Normal:** Single dense Linear comb-filter.
*   **Black HQ:** Dual-Tap Linear Ensemble (independent stereo decorrelation).

Whenever you want a different flavor of an existing color, try flipping the HQ switch. It will feel like an entirely new pedal.

---

## 6. Presets & Initialization

Choroboros ships with a default factory state. This state is a musically "safe" starting point (Rate at 0.5 Hz, Offset at 90°, Green Engine).

*   **DAW State Saving:** Whenever you save your project in Ableton, Logic, or Pro Tools, Choroboros silently serializes every knob position and Engine state into the DAW session file. When you re-open the project months later, Choroboros will sound exactly how you left it.
*   **Customization:** If you absolutely hate the Green engine and wish Choroboros always booted up on the Blue Engine with a heavy Rate, you can do this from the Dev Panel (see section 7).

---

## 7. The Hidden "DEV" Panel

If you are an advanced user—a sound designer, a C++ DSP student, or someone obsessed with telemetry data—you can open the hood.

Look at the top-right corner of the plugin. You'll see a small tab with a chevron arrow — this is the **TopBarDrawer**. Click it to expand the drawer, revealing icon buttons for DEV, About, Help, and Feedback. Click the **DEV** icon.

The entire UI will expand, revealing an immense secondary dashboard known as the **Dev Panel**. This exposes internal parameters and diagnostics, allowing you to reprogram the LFO phase integrators, tweak the BBD clock frequencies, stare at real-time Fast Fourier Transform spectral overlays, and write commands via an interactive console.

*(Warning: To learn how to operate the Dev Panel, please read `DEV_PANEL_MANUAL.md` and `DEV_PANEL_CONSOLE_MANUAL.md` located in the `/docs` installation directory).*

---

## 8. Glossary

*   **DSP:** Digital Signal Processing. The raw C++ mathematics manipulating your audio data.
*   **LFO:** Low Frequency Oscillator. An inaudible waveform (like a sine wave) used to automatically turn invisible knobs up and down rhythmically.
*   **Interpolation:** The mathematical guessing game an algorithm plays to figure out what a sound wave looks like *between* two digital samples. Crucial for smoothing delay lines.
*   **Phase Flanging/Comb Filtering:** When a delayed audio signal is extremely close (1-3ms) to the original signal, certain frequencies cancel each other out completely, creating deep "notches" in the EQ spectrum.
*   **BBD:** Bucket-Brigade Device. An old style of analog memory chip where voltage is passed down a line like firefighters passing buckets of water.
*   **GUI:** Graphical User Interface. The visual knobs and buttons you interact with.

*(End of Manual)*
