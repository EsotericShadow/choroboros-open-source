# Choroboros Dev Panel: Console & Tutorials Manual

Welcome to the Choroboros Dev Panel. This interface is built for power users, sound designers, and educators who want to modify the internal DSP mathematics of the plugin, trace signal flow, and learn about the analog-modeled components.

At the heart of the Dev Panel is the **Interactive Console**, located at the bottom of the Validation tab (and globally accessible via keyboard focus).

The console features a highly optimized C++ parser string-matching engine with aliasing, fuzzy search, latency telemetry, and deep introspection commands.

---

## 1. Parameter Slugification (The "Target")

To make setting parameters fast from the keyboard, all internal properties are mapped to a lowercase **slug** format. 
The parser removes special characters and converts spaces to underscores.

**Examples:**
* `"Rate Smooth (ms)"` ➔ `rate_smooth`
* `"Green Bloom Cutoff Max (Hz)"` ➔ `green_bloom_cutoff_max`
* `"Tape Drive Scale"` ➔ `tape_drive_scale`

### Global vs. Engine-Specific Scoping
If a parameter is completely specific to an engine (like `bbd_stages` or `purple_orbit_eccentricity`), it will only appear in the `list` output for that specific engine.
If a parameter applies across all engines (like `rate_smooth` or `hpf_cutoff_hz`), it is considered a global.

*Tip: If you don't know a parameter's slug, use the `search <term>` or `list <color>` commands.*

---

## 2. Command Dictionary

The console parses space-separated commands. Hit `Enter` to execute, and use the `Up Arrow` to cycle through your command history.

### 2.1 Engine & Plugin State
These commands modify the macro-level state of the plugin and UI context.

| Command | Action | Example | Detailed Behavior |
| :--- | :--- | :--- | :--- |
| `engine <color>` | Sets active engine. | `engine purple` | Immediately swaps the DSP profile. Colors: `green`, `blue`, `red`, `purple`, `black`. Safe to execute during audio playback; applies transactional crossfades to prevent zippering. |
| `hq <on/off>` | Toggles HQ mode. | `hq on` | Flips between the two algorithms built into every engine (e.g., Red NQ is BBD, Red HQ is Tape). |
| `view <tab_name>` | Switches right-column. | `view tape` | Valid tabs include: `overview`, `modulation`, `tone`, `engine`, `layout`, `validation`, `settings`, or specific sub-tabs like `bbd` or `tape`. |
| `bypass <on/off>` | Globally bypasses. | `bypass on` | Hard bypasses all DSP. The plugin outputs 100% dry signal. |
| `solo <node>` | Mutes all other paths. | `solo bbd_left` | Forces the DSP matrix to only pass the exact node string provided. Use `unsolo` to revert. |
| `unsolo` | Reverts solo routing. | `unsolo` | Restores normal DSP matrix execution paths. |

### 2.2 Parameter Tuning & Automation
These commands dictate the underlying mathematical behavior of the DSP engine and UI sliders.

| Command | Action | Example | Detailed Behavior |
| :--- | :--- | :--- | :--- |
| `set <target> <val>` | Sets a float/int value. | `set bbd_depth 12.5` | Bypasses UI scaling and writes the exact floating point value to the parameter memory. |
| `add <target> <val>` | Relative increase. | `add bbd_depth 2.0` | Takes the current value and mathematically adds `<val>` to it. Useful for bumping filter cutoffs. |
| `sub <target> <val>` | Relative decrease. | `sub rate_smooth 5` | Takes the current value and mathematically subtracts `<val>` from it. |
| `toggle <target>` | Flips boolean param. | `toggle tape_enabled` | If a parameter is 0, it becomes 1. If 1, it becomes 0. Fast way to flip switches. |
| `sweep <target> <start> <end> <ms>` | Smooth automation over time. | `sweep tape_drive 0 10 5000` | Asynchronously interpolates the parameter from the start value to the end value over the specified milliseconds (`ms`). The console updates at GUI refresh rate. Can be aborted by manually setting the parameter or typing another `sweep` on the same target. |
| `macro <name> <0-100>` | Simulates turning master knob. | `macro depth 75` | Maps onto the 6 master plugin knobs: `rate`, `depth`, `offset`, `width`, `color`, `mix`. Pass an integer 0 through 100 representing percentage. |
| `get <target>` | Prints current/previous value. | `get hpf_cutoff_hz` | Outputs the current exact float value AND tracks the value from before the last change. |
| `reset <target>` | Restores a single parameter. | `reset green_bloom_gain` | Snaps a targeted internal to the hardcoded factory default C++ schema value. |
| `reset all` | Nuke and pave everything. | `reset all` | Instantly resets **all** tuning, internal, and layout values to factory defaults. Use with caution. |
| `lock <target>` | Prevents UI modification. | `lock tape_drive` | Disables the UI slider for the target. Changes can still be forced via console `set` commands. |
| `unlock <target>` | Re-enables UI modification. | `unlock tape_drive` | Re-enables interaction with the slider. |

### 2.3 History & Undos
The console tracks every single parameter touch (via UI or Console) up to 256 actions deep.

| Command | Action | Example | Detailed Behavior |
| :--- | :--- | :--- | :--- |
| `undo` | Reverts last action. | `undo` | Restores the snapshot from before the last parameter adjustment or engine switch. |
| `undo <n>` | Reverts `n` actions. | `undo 3` | Undoes the last three actions in sequence. |
| `redo` | Restores undone action. | `redo` | Redoes the parameter adjustment or engine switch. |
| `redo <n>` | Restores `n` actions. | `redo 2` | Redoes the last two undone actions in sequence. |
| `history` | Prints history log. | `history` | Outputs a numbered, chronological list of your recent touches and console commands. |

### 2.4 Deep Introspection & Telemetry
These commands extract data from the running DSP without modifying audio.

| Command | Action | Example | Detailed Behavior |
| :--- | :--- | :--- | :--- |
| `dump <color>` | Prints all internal values. | `dump green` | Iterates over the memory block and prints every parameter value registered to the requested engine. |
| `diff factory` | Prints modified parameters. | `diff factory` | Runs a background cache check against the C++ default schema and prints ONLY parameters in the active engine that have been altered. |
| `search <term>` | Fuzzy search parameters. | `search delay` | Searches all slugs and display names and prints matches along with their current running value. |
| `watch <target>` | Pins telemetry HUD. | `watch lfo1_rate` | Pins the target property to the top edge of the console window. It updates at 30 fps to reflect live changes. Max 6 pins shown, unlimited tracked in background. |
| `unwatch <target>` | Un-pins telemetry HUD. | `unwatch lfo1_rate` | Removes the parameter from the HUD. |
| `stats` | Perf/CPU Telemetry. | `stats` | Prints real-time tracking of DSP block execution times, engine profile swap counts, LFO phase integrators, and callback allocations. Vital for profiling. |
| `list <color>` | Formatted param list. | `list purple` | Prints all parameters categorized under an engine. Returns paged outputs by default. |
| `list <color> full` | Bypasses paging caps. | `list purple full` | Prints the unabridged list of all parameters. |
| `list globals` | Shared param list. | `list globals` | Prints parameters that affect the entire plugin (e.g., Pre-Emphasis EQ, Master Compressors). |

### 2.5 Utilities & Scripting
These commands manage Dev Panel preferences, exports, and custom aliases.

| Command | Action | Example | Detailed Behavior |
| :--- | :--- | :--- | :--- |
| `alias <name> <command>` | Registers a custom shortcut. | `alias panic reset all` | Maps `name` to execute `command`. Future calls to `panic` will run `reset all`. |
| `export script` | Generates a preset script. | `export script` | Compiles every modified parameter into a newline-separated list of `set <target> <val>` commands. Copies output instantly to system clipboard. |
| `import script` | Executes a batch script. | `import script` | Triggers a file selector dialog to load and execute a `.txt` or `.choroscript` batch, setting all state instantly. |
| `cp json` | JSON export. | `cp json` | Dumps the Dev Panel's exact memory tree state into a minified JSON object and copies to system clipboard. |
| `save defaults` | Modifies startup state. | `save defaults` | Writes your exact current configuration to disk. Every time the plugin is newly instantiated, it will load these tuning values instead of factory ones. |
| `fx <0/1/2>` | Globals layout preset. | `fx 2` | 0 = Flat graphics, 1 = Animations On, 2 = Heavy Glassmorphism. |
| `clear` | Clears console text. | `clear` | Deletes the visual log output. |
| `help` | Prints cheat sheet. | `help` | Outputs the condensed quick-reference list of all commands. |

---

## 3. The Interactive Tutorial System

To teach users the fundamentals of DSP and analog circuitry modeling, we built a fully interactive guided tutorial system overlay. 

The tutorial system automatically routes you to the correct tabs, highlights specific UI components, forces engine configurations, and provides step-by-step explanatory HUDs.

**Core Commands:**
* `tutorial` — Starts the 6-module master walk-through.
* `tutorial next` — Advances to the next instructional step.
* `tutorial next section` — Skips the current granular lesson and jumps to the next major tutorial section.
* `tutorial exit` (or `skip`) — Instantly aborts the active tutorial.

### Granular Deep-Dive Lessons

You can trigger specific granular lessons directly from the console at any time:

#### `tutorial bbd` (Bucket-Brigade Lesson)
Forces the **Red NQ** engine. Walks through:
1. Bucket-Brigade Stages and memory limits.
2. Analog clock speeds.
3. High-frequency aliasing and "Clock Whine" noise induction.
4. The delicate balance of steep Anti-Alias and Reconstruction Filtering to hide analog artifacts without destroying clarity.

#### `tutorial tape` (Magnetic Tape Lesson)
Forces the **Red HQ** engine. Walks through:
1. Physical tape loop concepts.
2. Wow (slow, mechanical pitch drift from un-rounded tape reels).
3. Flutter (fast, jittery friction from tape heads).
4. Tone LPFs to simulate varying ages of magnetic media degradation.
5. Tape Drive nonlinear saturation curves.

#### `tutorial phase` (Stereo Width Lesson)
Walks through:
1. The Modulation visual scope.
2. Independent Left/Right Dual LFO integrators.
3. Phase offset alignment.
4. How 180-degree anti-phase creates massive stereo enveloping and brain localization illusion.

#### `tutorial bimodulation` (Complex Motion Lesson)
Forces the **Purple NQ (Phase Warp)** engine. Walks through:
1. Why sine LFOs become predictably fatiguing.
2. Bi-Modulation theory (multiplying two independent LFOs).
3. Setting asynchronous speeds (e.g., 1.0Hz and 0.37Hz) to prevent alignment.
4. Visualizing organic, non-repeating chorus cycles.

#### `tutorial saturation` (Harmonics Lesson)
Walks through:
1. Tone Tab transfer curve visualizations.
2. Contrasting aggressive digital Hard Clipping (Black Engine fuzz) vs analog Soft Clipping (Red Tape Tube-style limiting).
3. Odd vs Even order harmonic generation.

#### `tutorial envelope` (Dynamics Lesson)
Forces the **Green HQ (Bloom)** engine. Walks through:
1. Envelope Follower mechanic theory.
2. Transients, Attack, and Release detector speeds.
3. How to dynamically map audio amplitude to LFO depth to make the chorus "breathe" with the performance (ducking during transients, swelling during tails).

*(These tutorials can also be triggered visually from the Setting dropdown menu.)*

---

## 4. Complete List of Exposable Target Slugs

Below is the definitive reference for every DSP mapping variable you can access using the `set`, `get`, `add`, `sub`, and `sweep` commands.

### 4.1 Global Tuning & Filtering Variables
* `rate_smoothing_ms`
* `depth_smoothing_ms`
* `depth_rate_limit`
* `centre_delay_smoothing_ms`
* `centre_delay_base_ms`
* `centre_delay_scale`
* `color_smoothing_ms`
* `width_smoothing_ms`
* `hpf_cutoff_hz`
* `hpf_q`
* `lpf_cutoff_hz`
* `lpf_q`
* `pre_emphasis_freq_hz`
* `pre_emphasis_q`
* `pre_emphasis_gain`
* `pre_emphasis_level_smoothing`
* `pre_emphasis_quiet_threshold`
* `pre_emphasis_max_amount`
* `compressor_attack_ms`
* `compressor_release_ms`
* `compressor_threshold_db`
* `compressor_ratio`
* `saturation_drive_scale`

### 4.2 Green (Classic Bloom) Sub-Parameters
* `green_bloom_exponent`
* `green_bloom_depth_scale`
* `green_bloom_centre_offset_ms`
* `green_bloom_cutoff_max_hz`
* `green_bloom_cutoff_min_hz`
* `green_bloom_wet_blend`
* `green_bloom_gain`

### 4.3 Blue (Modern Focus) Sub-Parameters
* `blue_focus_exponent`
* `blue_focus_hp_min_hz`
* `blue_focus_hp_max_hz`
* `blue_focus_lp_max_hz`
* `blue_focus_lp_min_hz`
* `blue_presence_freq_min_hz`
* `blue_presence_freq_max_hz`
* `blue_presence_q_min`
* `blue_presence_q_max`
* `blue_presence_gain_max_db`
* `blue_focus_wet_blend`
* `blue_focus_output_gain`

### 4.4 Red (BBD / Analog Tape) Sub-Parameters
* `bbd_delay_smoothing_ms`
* `bbd_delay_min_ms`
* `bbd_delay_max_ms`
* `bbd_centre_base_ms`
* `bbd_centre_scale`
* `bbd_depth_ms`
* `bbd_clock_smoothing_ms`
* `bbd_filter_smoothing_ms`
* `bbd_filter_cutoff_min_hz`
* `bbd_filter_cutoff_max_hz`
* `bbd_filter_cutoff_scale`
* `bbd_clock_min_hz`
* `bbd_clock_max_ratio`
* `bbd_stages`
* `bbd_filter_max_ratio`
* `tape_delay_smoothing_ms`
* `tape_centre_base_ms`
* `tape_centre_scale`
* `tape_tone_max_hz`
* `tape_tone_min_hz`
* `tape_tone_smoothing_coeff`
* `tape_drive_scale`
* `tape_lfo_ratio_scale`
* `tape_lfo_mod_smoothing_coeff`
* `tape_ratio_smoothing_coeff`
* `tape_phase_damping`
* `tape_wow_freq_base`
* `tape_wow_freq_spread`
* `tape_flutter_freq_base`
* `tape_flutter_freq_spread`
* `tape_wow_depth_base`
* `tape_wow_depth_spread`
* `tape_flutter_depth_base`
* `tape_flutter_depth_spread`
* `tape_ratio_min`
* `tape_ratio_max`
* `tape_wet_gain`
* `tape_hermite_tension`

### 4.5 Purple (Experimental Phase/Orbit) Sub-Parameters
* `purple_warp_a`
* `purple_warp_b`
* `purple_warp_k_base`
* `purple_warp_k_scale`
* `purple_warp_delay_smoothing_ms`
* `purple_orbit_eccentricity`
* `purple_orbit_theta_rate_base_hz`
* `purple_orbit_theta_rate_scale_hz`
* `purple_orbit_theta_rate2_ratio`
* `purple_orbit_eccentricity2_ratio`
* `purple_orbit_mix1`
* `purple_orbit_stereo_theta_offset`
* `purple_orbit_delay_smoothing_ms`

### 4.6 Black (Linear / Intensity Ensemble) Sub-Parameters
* `black_nq_depth_base`
* `black_nq_depth_scale`
* `black_nq_delay_glide_ms`
* `black_hq_tap2_mix_base`
* `black_hq_tap2_mix_scale`
* `black_hq_second_tap_depth_base`
* `black_hq_second_tap_depth_scale`
* `black_hq_second_tap_delay_offset_base`
* `black_hq_second_tap_delay_offset_scale`

### 4.7 UI Settings Variables
* `settings_tutorial_hints`
* `settings_ui_text_size`
* `settings_ui_text_mode`
* `settings_theme_preset`
* `settings_accent_source`
* `settings_manual_accent`
* `settings_color_vision_mode`
* `settings_reduced_motion`
* `settings_large_hit_targets`
* `settings_strong_focus_ring`
* `settings_console_auto_scroll`
* `settings_console_timestamps`
* `settings_console_wrap_lines`
* `settings_console_max_lines`
* `settings_show_scope_hint_line`
* `settings_confirm_reset_factory`
* `settings_confirm_set_defaults`
