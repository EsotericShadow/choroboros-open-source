# Choroboros: The Official User Manual

*Version: 2.03-beta*
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
    *   3.3 Offset (Center Delay Target)
    *   3.4 Width (Stereo Phase & Correlation)
    *   3.5 Color (Spectral Filtering)
    *   3.6 Mix (Parallel Integration)
4.  **The Engine Core Selector (The Five Colors)**
    *   4.1 The Green Engine: Classic & Envelopes
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

Choroboros comes compiled as an AU, VST3, AAX, and Standalone application for macOS and Windows. 

1.  Place the provided plugins in your operating system's default plugin directories (e.g., `/Library/Audio/Plug-Ins/VST3` on Mac, or `C:\Program Files\Common Files\VST3` on Windows).
2.  Launch your DAW (Ableton Live, Logic Pro, FL Studio, REAPER, etc.) and perform a plugin scan.
3.  Load Choroboros onto an audio track or software synthesizer.

Upon first launch, you will see a sleek, dark interface with six primary controls and a bright colored orb in the top center. This orb dictates the "Engine Color." By default, it glows Green.

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
The **Offset** knob is arguably the most important structural control in Choroboros. It dictates the base "Center Delay" time of the wet signal before the Depth modulation even begins.
*   **Low Settings (0 - 5 ms):** Flanger territory. The wet signal is so close to the dry signal that deep phase cancellation occurs. Pushing the Rate and Depth up here will sound metallic, jet-engine-like, and physically close to the ear.
*   **Mid Settings (10 - 25 ms):** True Chorus territory. The classic 1980s Roland Dimension or Juno style thickening. The audio sounds doubled, rich, and wide.
*   **High Settings (30 - 60 ms):** Slapback territory. The delay is so long that your brain perceives it as a distinct, rhythmic echo rather than a thickened phase. Excellent for rhythmic guitar parts or bouncy synth arpeggios.

### 3.4 Width
The **Width** knob controls the stereo spread and LFO phase correlation between your left and right speakers.
*   **0% (Mono):** The Left and Right LFOs are perfectly locked in phase. They sweep up and down at the exact same millisecond. The chorus sits dead center in the stereo field.
*   **100% (Anti-Phase):** The Left and Right LFOs are pulled exactly 180-degrees apart. When the left speaker is pitching upwards, the right speaker is pitching downwards. This creates a colossal, hyper-wide stereo image that sounds like it is wrapping completely around the listener's head.

### 3.5 Color
The **Color** knob consolidates an entire rack of EQs, Pre-Emphasis filters, and transient shapers into one slider.
*   **0% (Dark):** Heavy low-pass filtering. High frequencies are destroyed before entering the chorus array, and high frequencies are crushed again when exiting. The resulting chorus is warm, muffled, and sits far back in the mix. Crucial for bass guitars to prevent high-end phase smearing.
*   **50% (Neutral):** Flat frequency response. What goes in is precisely what comes out.
*   **100% (Bright):** Extreme high-pass filtering and high-shelf EQ boosting. The low end is removed entirely to prevent muddy low-frequency phasing. The high frequencies are heavily amplified, creating a crystalline, airy, and shimmering top-end. Excellent for airy vocals or hi-hats.

### 3.6 Mix
The **Mix** knob dictates the parallel blend of the unprocessed (Dry) signal against the chorused (Wet) signal.
*   **0%:** 100% Dry. The plugin outputs identical audio to the input. (Note: The internal DSP actually shuts down at 0% to save valuable CPU overhead).
*   **50%:** Equal parts Dry and Wet. The thickest, most traditional chorus sound, allowing maximum comb-filtering phase interaction.
*   **100%:** 100% Wet. True vibrato. Because there is no Dry signal to reference against, you do not hear thick chorusing; you only hear the pure pitch-bending wobble of the delayed signal.

---

## 4. The Engine Core Selector (The Five Colors)

Clicking the glowing colored orb at the top center of the UI drops down the **Engine Core Selector**. This is the heart of Choroboros.

Changing the Engine doesn't just change parameters; it completely unloads the current C++ algorithm from memory and hot-swaps in a totally different mathematical topology. Choroboros is effectively five plugins in one.

### 4.1 The Green Engine (Classic Bloom)
*   **Vibe:** Natural, dynamic, acoustic.
*   **Implementation:** The Green engine is built on pristine 3rd-order Lagrange interpolation for smooth, artifact-free sweeping. 
*   **Secret Weapon (Envelope Follower):** Green is the only engine with an internal dynamic envelope follower. It listens to how loud you play. If you strum a guitar aggressively, the chorus Depth automatically scales up. If you play softly, it flattens out. It makes the chorus feel "alive" and reactive to human performance.

### 4.2 The Blue Engine (Modern Focus)
*   **Vibe:** Surgical, EDM, crystalline, aggressive.
*   **Implementation:** Blue uses insanely high-fidelity 5th-order Lagrange math combined with steep 8-pole IIR (Infinite Impulse Response) tracking filters.
*   **Secret Weapon (Presence EQ):** Blue is designed to cut through dense electronic mixes. Its "Color" knob manipulates an aggressive parametric bell-curve EQ that spikes presence frequencies, ensuring the chorus sits surgically on top of synth leads.

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
*   **Secret Weapon (Multi-Tap Ensemble):** Black doesn't just use one delay line; it uses multiple play-heads tapped off the same buffer (an Ensemble). This results in incredibly dense, metallic phase cancellation. It is designed specifically to make heavy distortion guitars sound impossibly massive.

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
*   **Black HQ:** 3-Voice Tap Ensemble array.

Whenever you want a different flavor of an existing color, try flipping the HQ switch. It will feel like an entirely new pedal.

---

## 6. Presets & Initialization

Choroboros ships with a default factory state. This state is mathematically proven to be a musically "safe" starting point (Moderate Rate, Offset at 15ms, Green Engine).

*   **DAW State Saving:** Whenever you save your project in Ableton, Logic, or Pro Tools, Choroboros silently serializes every knob position and Engine state into the DAW session file. When you re-open the project months later, Choroboros will sound exactly how you left it.
*   **Customization:** If you absolutely hate the Green engine and wish Choroboros always booted up on the Blue Engine with a heavy Rate, you can do this from the Dev Panel (see section 7).

---

## 7. The Hidden "DEV" Panel

If you are an advanced user—a sound designer, a C++ DSP student, or someone obsessed with telemetry data—you can open the hood.

Look at the very top left corner of the plugin. Hover your mouse over the tiny text reading `DEV`. Click it.

The entire UI will slide right, revealing an immense secondary dashboard known as the **Dev Panel**. This exposes all 119 internal float parameters, allowing you to reprogram the LFO phase integrators, tweak the BBD clock frequencies, stare at real-time Fast Fourier Transform spectral overlays, and write scripts via an interactive C++ console.

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
