# Choroboros Dev Panel: The Comprehensive UI and Architecture Manual

*Version: 2.02.2*
*Audience: Advanced Sound Designers, DSP Developers, Audio Engineers, and the terminally curious.*

Welcome to the ultimate guide for the **Choroboros Dev Panel**. If the standard interface of Choroboros is the dashboard of a luxury sports car, the Dev Panel is the act of opening the hood, dismantling the engine block, and reprogramming the Electronic Control Unit while driving at 100 mph. 

This document explicitly covers the **Visual UI, Inspector Columns, Tab Modules, and Mapping Architectures** of the Dev Panel. 

*(If you are looking for the Interactive C++ CLI commands, please see `DEV_PANEL_CONSOLE_MANUAL.md`).*

---

## Table of Contents
1.  **Philosophy & The "Why"**
2.  **Accessing the Panel**
3.  **The Master Control Header**
4.  **The Persistent Left Column: Mapping & Interpolation**
    *   4.1 Engine Architecture Hot-Swapping
    *   4.2 Parameter Limit Hardcapping
    *   4.3 Curve Skews and Polynomial Responses
    *   4.4 UI Drag Resistance
5.  **Right Deck Tab 0: The Overview Canvas**
    *   5.1 The Telemetry Hud
    *   5.2 The Dynamic Signal Router
6.  **Right Deck Tab 1: Modulation & Topology**
    *   5.1 Oscilloscope Fundamentals
    *   5.2 Trajectory Plotting
7.  **Right Deck Tab 2: Tone & Transfer**
    *   7.1 The Fast Fourier Transform Analyzer
    *   7.2 Nonlinear Saturation Curves
8.  **Right Deck Tab 3: DSP Internals Matrix**
    *   8.1 The Green "Bloom" Architecture
    *   8.2 The Blue "Focus" Architecture
    *   8.3 The Red "Analog Space" Architectures
    *   8.4 The Purple "Bi-Modulation" Architectures
    *   8.5 The Black "Hard Linear" Architectures
9.  **Right Deck Tab 4: Vector Graphics Layout**
10. **Right Deck Tab 5: Validation Inspector**
11. **Right Deck Tab 6: Preferences & Accessibility**
12. **Glossary of Terms**

---

## 1. Philosophy & The "Why"

Choroboros was designed to be a "Chorus that eats its own tail." It contains 10 totally distinct DSP chorus architectures grouped into 5 color-coded engines. However, standard plugins hide their math. If a chorus pedal has a "Depth" knob, you rarely know if 100% means a 2-millisecond stretch or a 30-millisecond stretch.

The Dev Panel exists to completely destroy that black-box paradigm. It was built on the core belief that **users deserve to see the math**.

Every single slider, knob, and pixel in the front-end C++ JUCE framework is bound to an APVTS (Audio Processor Value Tree State) atomic parameter. The Dev Panel hooks directly into this tree, allowing you to manipulate the raw floats before they hit the audio callback thread.

---

## 2. Accessing the Panel

Opening the Dev Panel is intentionally obfuscated to prevent beginner users from accidentally destroying a mix.

1.  Open Choroboros in your host DAW (Ableton, Logic, REAPER, FL Studio).
2.  Look to the top-left corner of the sleek minimalist UI.
3.  Hover your mouse precisely over the word `DEV`. A small tooltip will fade in confirming the hitbox.
4.  Click `DEV`.
5.  The UI will instantaneously expand horizontally, sliding open the complex Inspector and Data Decks.

*Note: The Dev Panel requires a minimum screen resolution width of 1200px to render comfortably without overlapping components.*

---

## 3. The Master Control Header

Along the absolute top rim of the Dev Panel sits an array of administrative utility buttons. These functions execute entirely on the message thread and handle global payload delivery.

### 3.1 Copy JSON State
This button serializes the current state of every single parameter—all tuning floats, all layout coordinates, and all macro mappings—into a minified JSON object and writes it directly to your operating system's clipboard. 
*   **Use Case:** Ideal for collaborating with other producers. Simply paste the JSON hash into an email or Discord, and they can inject your exact DSP architecture into their session.

### 3.2 Set Current As Defaults
Choroboros stores a lightweight `startup_prefs.xml` file in your application data directory. Clicking this button takes a snapshot of your current Dev Panel state and writes it to that XML.
*   **Use Case:** If you believe the factory default for the "Black" engine is slightly too aggressive, you can pull down the `depth` mapping, hit this button, and forevermore every new instance of Choroboros will load with your tamed Black engine.

### 3.3 The Master Lock
A physical padlock icon that, when engaged, recursively iterates through all `LockableFloatPropertyComponent` UI elements in the Dev Panel and disables mouse interaction. 
*   **Use Case:** You have spent 45 minutes perfectly tuning the specific Q-factor of a Thiran all-pass filter. You lock the panel so a stray trackpad swipe doesn't accidentally move the slider to maximum and blow out your monitors.

### 3.4 Glassmorphism FX Toggles (Off / Subtle / Medium)
Choroboros leans heavily into a modern UI aesthetic utilizing computationally expensive real-time drop shadows, gradients, and inner-glows (Glassmorphism). 
These buttons instantly alter the `juce::DropShadow` and bounding-box opacity math. Turning them "Off" forces a flat-vector aesthetic that significantly reduces GPU rendering overhead.

---

## 4. The Persistent Left Column: Mapping & Interpolation

The left-most column is fixed. It acts as the "Babel Fish" translation layer between human interaction on the main UI and machine math in the DSP loop.

### 4.1 Engine Architecture Hot-Swapping
The very top dropdown determines WHICH mathematical formula is evaluating audio on the realtime thread.
*   **Dropdown:** Selects the color matrix (`Green`, `Blue`, `Red`, `Purple`, `Black`).
*   **HQ Switch:** A boolean toggle. In standard plugins, "Over-sampling" is usually a simple linear 2x up-sample to prevent aliasing. In Choroboros, "HQ" acts as a literal engine swap.
    *   *Example:* Selecting `Red` + `HQ Off` loads the `ChorusCoreBBD.cpp` matrix.
    *   *Example:* Selecting `Red` + `HQ On` unloads the BBD matrix and hot-swaps in the `ChorusCoreTape.cpp` matrix. The entire acoustic topology changes.
*   **Target Scope:** Aesthetic changes (like shifting a knob X/Y coordinate) can be applied specifically to the active color, or broadcast to `ALL` colors.

### 4.2 Parameter Limit Hardcapping
Consider the front-facing **Rate** knob. It has a physical travel from 0 to 100%. But what does 100% mean?
In this column, you will see `Min` and `Max` text entry boxes for the 6 primary macros: `Rate`, `Depth`, `Offset`, `Width`, `Color`, `Mix`.
*   If you set the Rate `Min` to `0.1 Hz` and `Max` to `5.0 Hz`, tracing the front-side GUI knob maps linearly between those two floats.
*   Moving the knob to 50% yields `2.55 Hz`. 

### 4.3 Curve Skews and Polynomial Responses
Humans don't perceive audio linearly. A change in LFO speed from 0.1Hz to 1.0Hz feels massive. A change from 10Hz to 11Hz is almost imperceptible.
The `Curve` text entry modifies the interpolation skew.
*   `1.0`: Linear mapping.
*   `< 1.0`: The bottom half of the knob travel covers very little numerical ground, providing extreme fine-tuning. The top half of the knob accelerates rapidly.
*   `> 1.0`: The bottom half jumps aggressively, flattening out at the top.

### 4.4 UI Drag Resistance
Listed under "UI Response", these parameters do not change DSP math. They change OS mouse-pixel deltas.
*   A `UI Skew` of `1.0` means moving your mouse 100 pixels up turns the knob 100%.
*   Lowering the skew means you have to drag your mouse halfway across the monitor just to move the knob 10%. Excellent for users with high-DPI gaming mice who want stiff, resistant knobs.

---

## 5. Right Deck Tab 0: The Overview Canvas

Clicking the `Overview` tab brings up the macro HUD. It is designed to be the "Dashboard" of your DSP engine.

### 5.1 The Telemetry HUD
Along the top are absolute readouts reflecting the exact state variables on the audio thread.
*   **True HPF/LPF Effective:** If your `Color` macro knob is set to 42%, what exact frequency (`Hz`) is the internal Biquad filter currently blocking? This telemetry prints the exact float.
*   **Latency (Samples):** Every chorus algorithm incurs inherent delay. Linear Phase EQs or complex Orbit paths might push latency to 1024 samples. Your DAW needs to know this for Plugin Delay Compensation (PDC). This tells you what Choroboros is reporting to the host.

### 5.2 The Dynamic Signal Router
A massive, horizontal block diagram. 
*   **Blocks:** Visualizes `Input -> Pre-Emphasis -> Phase Topology -> Chorus Modulator -> Saturation Engine -> Output`.
*   **Badges:** Small rectangular chips sit beneath the blocks inside the diagram. If you pull the `Mix` knob to 0%, you will see the Chorus Modulator badge physically turn grey and read `[BYPASSED]`, proving to you visually that the DSP branch has shut down to save CPU.
*   If you change an internal filter from 400Hz to 800Hz, the text inside the badge updates in real time.

---

## 6. Right Deck Tab 1: Modulation & Topology

This tab visualizes Time and Phase. A chorus works by delaying an audio signal and then modulating that delay duration using an LFO (Low Frequency Oscillator).

### 6.1 Oscilloscope Fundamentals
The top graph is the `Phase Scope`.
*   It traces two lines simultaneously: The **Left Channel** phase and the **Right Channel** phase over the last 1-second rolling window.
*   When `Width` is at 0%, the lines overlap perfectly.
*   As you increase the `Offset` tuning mapping, you will physically see the Right channel wave separate and lag behind the Left channel. When they are directly opposite (Anti-Phase, 180 degrees), the chorus will sound incredibly wide, almost wrapping behind your head in headphones.

### 6.2 Trajectory Plotting
The bottom graph is the `Delay Trajectory` sparkline. 
*   The Phase Scope only shows the *multiplier* (-1.0 to 1.0). 
*   The Trajectory plot shows the *actual delay time* in `ms`. 
*   If you set the Center Delay base tuning to `15.0ms`, and the Depth to `5.0ms`, you will visually see the Trajectory graph oscillating like a sine wave between a floor of `10.0ms` and a ceiling of `20.0ms`. This proves that the math is bound correctly.

---

## 7. Right Deck Tab 2: Tone & Transfer

This tab visualizes Frequency and Amplitude.

### 7.1 The Fast Fourier Transform Analyzer
The top window is a high-speed, perceptual FFT analyzer.
*   **Thread Safety:** The analyzer never blocking the audio thread. The DSP pushes sample chunks into a lock-free circular FIFO buffer. A background GUI thread eats from this buffer, applies a Hann window, runs a high-performance FFT, and plots the magnitude log-scaled.
*   **Overlays:** Overlaying the FFT data are thick vector lines representing your EQ shapes. As you turn down the `Color` knob to darken the chorus, you will see a solid Line (the Low-Pass filter) sweep down from the right side of the graph, visually "crushing" the high-frequency FFT chatter.

### 7.2 Nonlinear Saturation Curves
The bottom window is the `Transfer Curve`. 
*   **The X-Axis** is an audio sample's amplitude coming IN to the saturation block (-1.0 to 1.0).
*   **The Y-Axis** is the amplitude coming OUT.
*   **Digital Clean:** A perfect diagonal line from bottom-left to top-right. Input perfectly equals Output.
*   **Soft Clipping (Tape):** As you push the `Drive` tuning parameter up, you will see the top and bottom tips of the diagonal line start to round off smoothly. This is analog tube/tape behavior. It compresses the signal gently, generating warm even-order harmonics.
*   **Hard Clipping (Black):** The diagonal line goes up, and then instantly snaps perfectly flat at a harsh angle. This is digital fuzz/distortion. It chops off audio peaks, generating aggressive odd-order harmonics that cut through an incredibly dense metal/rock mix.

---

## 8. Right Deck Tab 3: DSP Internals Matrix

This is the most dangerous, unstable, and rewarding tab. This is where you actually reprogram the C++ algorithms. The parameters visible here dynamically swap depending on which Engine you selected in the Left Column.

Below is an exhaustive explanation of the architectural families you can tune here.

### 8.1 The Green "Bloom" Architecture
*Target Demographic: Orchestral arrangers, dynamic jazz guitarists.*
The Green engine utilizes an internal "Envelope Follower." It listens to how loud the incoming audio is (e.g. tracking the transient spike of a snare drum vs the tail ring).
*   **Detector Speeds (Attack/Release):** Tune how fast the internal listener reacts to volume changes. A 5ms attack snaps instantly to transients. A 500ms release lets the internal state sink away glacially over half a second.
*   **Depth Scale/Gain:** You can map the Envelope Follower's output directly to the Chorus Depth. As the player strums harder, the chorus becomes violently detuned. When the player picks softly, the chorus flattens out to a pristine delay line. 
*   **Cutoff Tracking:** You can actually tell the high-pass filter to open and close dynamically with the volume envelope, essentially turning the chorus into an auto-wah envelope filter embedded within a modulation topography.

### 8.2 The Blue "Focus" Architecture
*Target Demographic: EDM producers, surgical mix engineers.*
The Blue engine is mathematically pristine. It utilizes 5th-order Lagrange interpolation to ensure crystalline, artifact-free high-frequency reproduction, and focuses heavily on aggressive phase-aligned spectral filtering.
*   **Presence EQs:** Instead of a simple "tone" knob, the Blue engine exposes a parametric bell curve. You can dial an exact `Presence Frequency` (e.g. 5000Hz), apply an exact `Q` width (e.g. 0.707), and push a `Gain dB` spike.
*   **HP/LP Bricks:** The cutoffs on Blue are sheer walls. Tuning the `HP Min/Max` here dictates the floor of the steep 8-pole IIR (Infinite Impulse Response) tracking filters.

### 8.3 The Red "Analog Space" Architectures
*Target Demographic: Synthwave artists, vintage gear aficionados, Lo-Fi beatmakers.*
Red completely abandons digital perfection for emulated hardware degradation.

#### Red NQ: The Bucket-Brigade (BBD) Device
An emulation of dark, noisy 1970s analog delay chips.
*   **BBD Stages:** A physical BBD chip has a finite memory buffer (stages). You can manually tune this to 512, 1024, or 4096. 
*   **Clock Minimum Frequency:** The slower you pass audio buckets, the longer the chorus delay. But pulling the clock speed too low causes horrific "Clock Whine" noise induction.
*   **Anti-Alias Filters:** To hide the clock whine, you must aggressively tune steep Low Pass filters before the buffer to destroy highs, and steep Reconstruction filters after the buffer to smooth the jagged voltage drops. The Dev Panel lets you ruin those filters so the clock screech is audible for terrifying sound design.

#### Red HQ: The Magnetic Tape Machine
An emulation of the legendary Roland Space Echo loops.
*   **Wow Frequency & Depth Space:** "Wow" is the slow, seasick pitch-variation caused by warped, asymmetrical tape reels. You map exactly how fast the reel wobbles (Hz) and how much it drags the pitch (cents).
*   **Flutter Frequency:** "Flutter" is the fast, rapid vibration caused by friction as magnetic tape scrapes against a dirty play-head. 
*   **Hermite Tension:** Switches the math interpolation to Dark Hermite splines, artificially rounding off transients dynamically to create "tape warmth" independent of saturation curves.

### 8.4 The Purple "Bi-Modulation" Architectures
*Target Demographic: Ambient soundscapers, cinematic drone designers.*
Normal choruses use a boring, predictable sine wave that repeats exactly every cycle. Purple destroys predictability using dual asynchronous phase integrators.

*   **Theta Rate Ratios:** The purple engine runs **two** LFOs multiplied together. Here, you tune the exact mathematical ratio between LFO 1 and LFO 2. If you set them to prime-number asynchronous ratios (e.g. 1.0Hz and 0.37Hz), the wave cycles will not align for minutes at a time, creating an organic, non-repeating chorus motion.
*   **Eccentricity:** In "Orbit" mode, the dual LFOs map to an X/Y polar graph, like a planet orbiting a star. Tuning the `eccentricity` pushes the orbit from a perfect circle into an extreme elliptical smash, causing brief bursts of hyper-speed modulation followed by long swaths of stillness.

### 8.5 The Black "Hard Linear" Architectures
*Target Demographic: Nu-Metal bassists, Industrial Techno producers.*
Black is designed to be abrasive, phase-coherent, and comb-filtering heavily.

*   **Ensemble Tap Ratios (HQ):** Classic choruses only have one delay line per ear. Black HQ uses multiple play-heads tapped off the same delay line (Ensemble). 
*   **Tap2 Delay Offset Base:** You can tune exactly how many milliseconds the second play-head sits behind the primary head, creating a thick, metallic comb-filter phase cancellation that makes heavy-distortion guitars sound gargantuan.
*   **Delay Glide Smoothing:** Black drops its interpolation fidelity to absolute basic Linear rendering, causing high-frequency aliasing hash. Tuning the glide forces hard mathematical snapping instead of smooth curving.

---

## 9. Right Deck Tab 4: Vector Graphics Layout

The Layout tab is not for audio; it is purely a GUI designer space.

Every primary knob on the front face of the plugin (Rate, Depth, Offset, Width, Color, Mix) has a fixed location per Engine.
*   If you select `Purple` engine, you can edit the `X` and `Y` coordinate text boxes for the `Depth` knob. 
*   As you type new numbers, the knob physically slides across the screen. 
*   You can utilize this to build your own custom GUI configurations where the knobs sit in a straight line, a circle, or clumped chaotically in a corner.
*   Hit `Set Current As Defaults` on the top bar to permanently save your custom visual arrangement.

---

## 10. Right Deck Tab 5: Validation Inspector

This tab is primarily for the developers of the plugin to ensure that visual widgets are accurately mapping into the deep-threaded architecture.
*   **UI Value -> Mapped Value -> Snapshot Value -> DSP Value:** A massive tracking matrix. It literally takes the 0.0-1.0 float of the UI slider, runs it through your specified Curve Skew algorithm, prints the resulting `Mapped Float`, shows the atomic variable value stored in the thread-safe `APVTS` state tree, and then verifies that the active audio `processBlock()` is actually receiving that exact float.
*   If any of these columns disagree, there is a thread-safety or interpolation bug in the Matrix.

---

## 11. Right Deck Tab 6: Preferences & Accessibility

The settings that dictate the usability of the Dev Panel itself.

### 11.1 Tutorial Launcher
A dropdown containing the 6 built-in C++ interactive script lessons.
Selecting one and clicking "Run" will trigger a translucent HUD overlay that automatically flips the Dev Panel tabs, highlights UI elements with glowing borders, forces engine architectures, and provides paragraph explanations of what DSP phenomena you are looking at.

### 11.2 Theme Engines
*   **Engine Adaptive (Default):** The Dev Panel's background colors, text tints, and slider fills all shift based on whether you are analyzing the Green, Blue, Red, Purple, or Black engine.
*   **Classic Hacker:** A sleek, minimal, low-emission Green-on-Pitch-Black terminal aesthetic.
*   **High Contrast:** Strips transparency algorithms, bumps font weights to maximum thickness, and uses pure black/white/yellow warning colors for visually impaired operators.

### 11.3 Vision Mode Correction
This shifts the base hue algorithms of the 5 main specific engines.
If you suffer from Deuteranopia, standard Green and Red algorithms might become indistinguishable. These toggles shift the GUI render pipeline to scientifically color-blind safe palettes without altering the naming convention of the engines.

### 11.4 Animation & Motion Guardrails
*   **Reduced Motion:** Disables the smooth kinetic-scrolling equations on sliders, disables tab-sliding crossfades, and forces instantaneous UI rendering to prevent vestibular distress.
*   **Large Hit Targets:** Triples the invisible clickable bounding box radius around sliders and toggles for operators using trackballs or stylus interfaces.
*   **Strong Focus Rings:** Specifically for Screen Reader compliance. Forces a harsh 3-pixel bright yellow dashed border around whichever widget currently possesses keyboard focus tab-cycling.

---

## 12. Glossary of Terms
*   **APVTS:** Audio Processor Value Tree State. JUCE's thread-safe atomic data warehouse for VST/AU parameter syncing.
*   **BBD:** Bucket-Brigade Device. An analog chip that passes capacitance voltage down a chain sequentially to create delay lines.
*   **LFO:** Low Frequency Oscillator. A math wave (sine, triangle) running too slow to hear as pitch, used to automate other parameters.
*   **IIR:** Infinite Impulse Response. Filters that use recursive feedback, creating analog style phase-warping at the cutoff point.
*   **HQ:** High-Quality. A toggle switch in Choroboros that swaps entire codebase algorithms (e.g. BBD vs Tape).

*(End of Manual)*
