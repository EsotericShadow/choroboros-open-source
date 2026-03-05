# Choroboros Dev Panel: The Complete User Manual

Welcome to the Choroboros Dev Panel. This interface is built for power users, sound designers, and educators who want to modify the internal DSP mathematics of the plugin, trace signal flow, and learn about the analog-modeled components.

**How to open:** Click the **DEV** button in the top-left corner of the plugin GUI.

---

## 1. Top Bar Globals

Along the top navigation bar of the Dev Panel, you have access to core global functions:
*   **Copy JSON:** Copies all current Dev panel internal values as a minified JSON object to your clipboard.
*   **Set Current as Defaults:** Saves all tuning, internals, and UI layout settings as your startup defaults. Every time you open a new instance of the plugin, it will load these custom states instead of factory values.
*   **Lock:** A global toggle that disables modification of all visual Inspector UI sliders to prevent accidental clicks. (You can still set values via the Console while locked).

---

## 2. Left Column: The Inspector

The left column of the Dev Panel is the **Inspector**. It is always visible regardless of which right-hand tab you have open.

### Engine Architecture Selectors
*   **Engine:** A dropdown to instantly swap the active DSP profile mathematically (`Green`, `Blue`, `Red`, `Purple`, `Black`).
*   **HQ Toggle:** Enables High Quality algorithms for each engine (e.g., Red NQ is Bucket-Brigade, Red HQ is Tape).
*   **Apply to All Target:** A utility allowing you to broadcast aesthetic Layout changes to all engines at once.

### Parameter Mapping (Raw to APVTS)
This section controls how the front-side GUI macro knobs (`Rate`, `Depth`, `Offset`, `Width`, `Color`, `Mix`) map their 0.0-1.0 percentage values into the actual DSP algorithms. Have you ever wished the "Rate" knob capped out at 5Hz instead of 20Hz so you'd have more granular control? Change the `Max` mapping here.
*   **Min / Max:** Definitively hardcaps the floor and ceiling values passed into the DSP.
*   **Curve:** The skew curvature of the mapping. A curve of 1.0 is linear. A curve of 0.5 makes the first half of the knob sweep very sensitive, while the second half covers a massive range rapidly.

### UI Response
Changes the physical "feel" of dragging the Master knobs on the front panel. Altering the `Skew` makes the mouse sensitivity non-linear.

---

## 3. Right Column: The Visual Decks & Tabs

The right column houses the deeply introspective visualizers and tuning properties, broken down by category.

### Tab 1: Overview
The macro view of your active profile.
*   **Signal Flow Diagram:** A live block-diagram showing the exact routing chain from Input -> Pre-Emphasis -> Filters -> Chorus Array -> Saturation -> Output.
*   **Meters:** Live input (Pre), effect (Wet), and output (Post) level dB meters.
*   **Derived Readouts:** Numeric readouts showing physically derived states, such as true HPF/LPF Cutoffs, internal sample Latency, or the Red NQ BBD Minimum Delay in milliseconds given the current physical clock limits.

### Tab 2: Modulation
The temporal inspection suite. Unlocks the mysteries of phase, offset, and dual-LFOs.
*   **LFO Scope:** Realtime oscilloscope drawing the Left and Right LFO waveforms propagating towards the delay lines. Essential for tuning the Purple bi-modulation shapes or visualizing `Width Offset` anti-phase.
*   **Trajectory Sparklines:** Shows the actual delay-time modulation in milliseconds over time. This makes the `Depth` knob's effect tangible.

### Tab 3: Engine Response (Tone)
The spectral and nonlinear behavioral inspection suite.
*   **Tone Transfer Curve:** A graph showing Input Amplitude vs. Output Amplitude. Perfect for visualizing the difference between Black's aggressive 'Hard Clipping' brick wall, and Red's smooth analog 'Soft Clip' saturation.
*   **Spectrum Analyzer:** A high-speed, background-thread FFT analyzer overlaid with the true shapes of your active High-Pass, Low-Pass, and Pre-Emphasis filters.

### Tab 4: DSP Internals
The heart of the Dev Panel. This tab is contextually aware: if you select the `Red NQ` engine, it will ONLY show you tuning parameters for Bucket-Brigade chips.
*   **Filter String:** Filter by parameter name.
*   **Show Advanced:** Discloses deeper mathematical properties like crossfade smoothing values (`bbd_delay_smoothing_ms`) or esoteric hardware limits (`bbd_clock_max_ratio`).
*   Depending on the engine, you will tune specific stages: Filter stages, Compressor attack/releases, Tape Wow/Flutter speeds, BBD clock frequencies, Phase Warp limits, or Ensemble mix targets.

### Tab 5: Layout (UI Aesthetics)
Separates UI design from DSP math. Move the main knobs on the front UI visually by adjusting their X/Y coordinates, apply JetBrains typographies, and tune the glassmorphism Glow/Reflection FX scalars per engine.

### Tab 6: Validation Console
The execution and wiring-check module. The top half shows live APVTS tracking paths. The bottom half is the **Interactive C++ Console** (see below).

### Tab 7: Settings
Granular UI and accessibility preferences.
*   **Tutorial Module Trigger:** Select a guided tutorial (e.g. `Tape`, `Phase`) from the dropdown and click Run.
*   **Themes:** Swap from Engine Adaptive (dark mode syncing to engine color) to Classic Hacker (Green on Black) or High Contrast.
*   **Color Vision Paths:** Shifts the engine color rendering for Protanopia, Deuteranopia, or Tritanopia users.
*   **Motion & Focus:** Toggles large hit targets, disables animations, and sets strong focus rings for screen reader navigation.

---

## 4. Interactive Console & Commands

To make setting parameters fast from the keyboard, all internal properties in the Dev Panel are mapped to a lowercase **slug** format (e.g. `"Green Bloom Cutoff Max (Hz)"` ➔ `green_bloom_cutoff_max`).

The console parses space-separated commands. Hit `Enter` to execute, and use the `Up Arrow` to cycle through your command history.

### 4.1 Engine & Plugin State
| Command | Action | Detailed Behavior |
| :--- | :--- | :--- |
| `engine <color>` / `hq <on/off>` | Sets active engine/HQ mode. | Immediately swaps the DSP profile with transactional crossfades to prevent zippering. |
| `view <tab_name>` | Switches right-column. | Valid tabs: `overview`, `modulation`, `tone`, `engine`, `layout`, `validation`, `settings`. |
| `bypass <on/off>` | Globally bypasses. | Hard bypasses all DSP. The plugin outputs 100% dry signal. |
| `solo <node>` / `unsolo` | Mutes all other paths. | Forces the DSP matrix to only pass the node string. `unsolo` restores routing. |

### 4.2 Parameter Tuning & Automation
| Command | Action | Detailed Behavior |
| :--- | :--- | :--- |
| `set <target> <val>` | Sets a float/int value. | Bypasses UI scaling and writes exact float to memory (e.g. `set bbd_depth 12.5`). |
| `add` / `sub <target> <val>` | Relative offset. | Mathematically adds or subtracts `<val>` from current value. |
| `toggle <target>` | Flips boolean param. | Instantly flips any 0/1 parameter. |
| `sweep <target> <start> <end> <ms>`| Smooth automation. | Asynchronously interpolates the parameter from start to end over `ms`. |
| `macro <name> <0-100>` | Master knob turn. | Maps to the 6 master plugin knobs (`rate`, `depth`, `offset`, etc). |
| `get <target>` | Prints current value. | Outputs exact float value AND tracks value from before last change. |
| `reset <target>` / `reset all` | Restore to factory. | `reset all` instantly nukes all tuning/UI values back to C++ defaults. |
| `lock` / `unlock <target>` | Toggles UI slider lock. | Disables slider interaction. Changes can still be forced via console `set`. |

### 4.3 Deep Introspection & Telemetry
| Command | Action | Detailed Behavior |
| :--- | :--- | :--- |
| `watch <target>` | Pins telemetry HUD. | Pins the property value to top of console window, updating at 30 fps. |
| `unwatch <target>` | Un-pins telemetry. | Removes the parameter from the HUD. |
| `stats` | Perf/CPU Telemetry. | Prints block execution times, swap counts, integrator values, allocations. |
| `dump <color>` | Prints all internals. | Dumps every parameter value registered to requested engine. |
| `diff factory` | Prints modifieds. | Checks cache against defaults and prints ONLY explicitly altered parameters. |
| `search <term>` | Fuzzy search. | Searches all slugs and descriptions for substring matches. |
| `list <color>` / `globals` | Formatted param list.| Prints all parameters in scope. Append `full` to bypass paging caps. |

### 4.4 History & Utilities
| Command | Action | Detailed Behavior |
| :--- | :--- | :--- |
| `undo` / `redo <n>` | Reverts actions. | Undoes/redoes the last `n` parameter adjustments or engine switches. |
| `history` | Prints history log. | Outputs numbered, chronological list of recent touches. |
| `alias <name> <command>`| Custom shortcut. | Maps `name` to execute `command` (e.g. `alias panic reset all`). |
| `export script` | Generates text preset. | Outputs newline-separated `set <target> <val>` commands to clipboard. |
| `import script` | Executes batch script. | Executes clipboard/file commands instantly to configure all states. |
| `cp json` | JSON export. | Copies entire Dev Panel state as minified JSON to clipboard. |
| `save defaults` | Modifies startup state.| Writes current configuration to disk to load via DAWs by default. |

---

## 5. The Interactive Tutorial System

If you type `tutorial` into the console, Choroboros launches a 6-module interactive guided walkthrough. The HUD overlay will physically route you to the correct tabs, highlight components, force engine configurations, and provide step-by-step DSP instruction.

You can also launch granular lessons directly from the console:
*   `tutorial bbd` — The Bucket-Brigade Lesson. Forces Red NQ (BBD Stages, Clock Whine, Anti-Alias filtering).
*   `tutorial tape` — The Magnetic Tape Lesson. Forces Red HQ (Mechanical wow/flutter, Tone LPFs, Drive saturation).
*   `tutorial phase` — The Stereo Width Lesson. (Dual LFO anti-phase independence).
*   `tutorial bimodulation` — The Complex Motion Lesson. Forces Purple NQ (Asynchronous Bi-Modulation theory).
*   `tutorial saturation` — The Harmonics Lesson. (Hard clipping vs Soft clipping transfer curves).
*   `tutorial envelope` — The Dynamics Lesson. Forces Green HQ (Generative Envelope Followers). 

*Tutorial commands: `tutorial next`, `tutorial next section`, `tutorial skip`.*
