# Choroboros Dev Panel: Console & Tutorials Manual

Welcome to the Choroboros Dev Panel. This interface is built for power users, sound designers, and educators who want to modify the internal DSP mathematics of the plugin, trace signal flow, and learn about the analog-modeled components.

At the heart of the Dev Panel is the **Interactive Console**, located at the bottom of the Validation tab (and globally accessible via keyboard focus).

---

## 1. Parameter Slugification (The "Target")

To make setting parameters fast from the keyboard, all internal properties are mapped to a lowercase **slug** format.
You use these slugs as the `<target>` when running tuning commands.

**Examples:**
* `"Rate Smooth (ms)"` ➔ `rate_smooth`
* `"Green Bloom Cutoff Max (Hz)"` ➔ `green_bloom_cutoff_max`
* `"Tape Drive Scale"` ➔ `tape_drive_scale`

*Tip: If you don't know a parameter's slug, use the `search` or `list` commands.*

---

## 2. Command Dictionary

The console parses space-separated commands. Hit `Enter` to execute, and use the `Up Arrow` to cycle through your command history.

### Engine & Plugin State
| Command | Description | Example |
| :--- | :--- | :--- |
| `engine <color>` | Switches the active DSP engine profile (`green`, `blue`, `red`, `purple`, `black`). | `engine purple` |
| `hq <on/off>` | Enables or disables HQ (High Quality) mode for the current engine. | `hq on` |
| `view <tab_name>` | Instantly switches the DevPanel right-column view (`internals`, `bbd`, `tape`, `layout`, `validation`, `settings`). | `view tape` |
| `bypass <on/off>` | Globally bypasses all DSP processing. | `bypass on` |

### Parameter Tuning
| Command | Description | Example |
| :--- | :--- | :--- |
| `set <target> <val>` | Sets a parameter to the specified float or int. | `set bbd_depth 12.5` |
| `add <target> <val>` | Increases a parameter by a specific amount. | `add bbd_depth 2.0` |
| `sub <target> <val>` | Decreases a parameter by a specific amount. | `sub rate_smooth 5` |
| `toggle <target>` | Instantly flips any boolean parameter (0/1). | `toggle tape_enabled` |
| `sweep <target> <start> <end> <ms>` | Smoothly automates a parameter from start to end over a specific duration (ms). | `sweep tape_drive 0.0 10.0 5000` |
| `macro <name> <val>` | Simulates turning one of the 6 main UI master knobs (`rate`, `depth`, `offset`, `width`, `color`, `mix`) to a specific percentage (0-100). | `macro depth 75` |
| `get <target>` | Prints the current value of the parameter AND what its previous value was. | `get green_bloom_exponent` |
| `reset <target>` | Restores the specified parameter to its hardcoded factory default. | `reset green_bloom_gain` |
| `reset all` | Instantly resets **all** tuning, internal, and layout values to factory defaults. | `reset all` |
| `lock <target>` | Locks the UI slider for the targeted parameter. | `lock tape_drive_scale` |
| `unlock <target>` | Unlocks the UI slider for the targeted parameter. | `unlock tape_drive_scale` |

### History & Undos
| Command | Description | Example |
| :--- | :--- | :--- |
| `undo` / `undo <n>` | Undoes the last parameter change(s) or command action(s). | `undo 3` |
| `redo` / `redo <n>` | Redoes the last undone parameter change(s). | `redo 2` |
| `history` | Prints a numbered list of your recent touches and console commands. | `history` |

### Introspection & Utilities
| Command | Description | Example |
| :--- | :--- | :--- |
| `dump <color>` | Prints all current internal DSP parameter values for the specified engine. | `dump green` |
| `diff factory` | Prints only the parameters in the active engine that currently differ from factory defaults. | `diff factory` |
| `search <term>` | Fuzzy-searches parameter names/descriptions and prints their current values. | `search delay` |
| `watch <target>` | Pins a parameter to a persistent HUD readout at the top of the console. | `watch lfo1_rate` |
| `unwatch <target>` | Removes the parameter from the pinned HUD. | `unwatch lfo1_rate` |
| `solo <node>` | Instantly mutes all other DSP paths and only outputs the selected node. | `solo bbd_left` |
| `unsolo` | Returns audio routing to normal. | `unsolo` |
| `stats` | Prints live telemetry (CPU time, callbacks, parameter write counts, profile swaps). | `stats` |
| `list <color>` / `globals` | Prints a formatted list of all available parameter slugs for a specific engine or global scope. | `list purple` |
| `export script` | Generates a list of `set` commands representing the current state and copies to clipboard (for easy preset sharing). | `export script` |
| `import script` | Opens a file dialog (or reads clipboard) to execute a batch of console commands instantly. | `import script` |
| `cp json` | Copies the total DevPanel state as JSON to the clipboard. | `cp json` |
| `save defaults` | Saves the entire current DevPanel state as the startup default profile. | `save defaults` |
| `clear` | Clears the console output history. | `clear` |
| `help` | Prints a helpful summary of all available core commands. | `help` |

---

## 3. The Interactive Tutorial System

To help users learn the fundamentals of DSP and analog circuitry modeling (and see how the DevPanel visualizes them), we have built a fully interactive guided tutorial system overlay.

The tutorial system automatically routes you to the correct tabs, highlights specific UI components, forces engine configurations, and provides step-by-step explanatory HUDs.

### Tutorial Commands

| Command | Action |
| :--- | :--- |
| `tutorial` | Starts the master walk-through (Overview -> Modulation -> Engine -> Tone -> Validation). |
| `tutorial exit` | Instantly aborts the active tutorial. |
| `tutorial skip` | Instantly aborts the active tutorial. |
| `tutorial next` | Advances to the next step. |
| `tutorial next section` | Skips the current granular lesson and jumps to the next major tutorial section. |

### Available Deep-Dive Lessons

You can trigger these specific granular lessons directly from the console at any time:

*   `tutorial bbd` — The Bucket-Brigade Lesson. Forces the Red NQ engine and walks through stages, clock speeds, high-frequency "Clock Whine", and the delicate balance of Anti-Alias filtering.
*   `tutorial tape` — The Magnetic Tape Lesson. Forces the Red HQ engine and explores physical tape degradation, Wow (pitch drift), Flutter (jitter), Tone LPFs, and Tape Drive nonlinear saturation.
*   `tutorial phase` — The Stereo Width Lesson. Explores the Modulation scope, dual LFO independence, and how 180-degree anti-phase creates massive stereo enveloping.
*   `tutorial bimodulation` — The Complex Motion Lesson. Forces the Purple NQ (Phase Warp) engine and visualizes how multi-LFO multiplication creates organic, non-repeating chorus cycles.
*   `tutorial saturation` — The Harmonics Lesson. Explores the Tone Tab transfer curves, contrasting the aggressive Hard Clipping (Black engine) with analog Soft Clipping (Red Tape engine) and highlighting odd-order harmonic generation.
*   `tutorial envelope` — The Dynamic Lesson. Forces the Green HQ engine to teach the mechanics of an Envelope Follower, Attack/Release detectors, and how to dynamically map source audio volume to LFO depth to make the chorus "breathe" with the performance.

You can also trigger these tutorials visually from the **Settings** tab in the DevPanel.
