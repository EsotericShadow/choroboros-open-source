# Choroboros Dev Panel: Interactive Console Manual

*Version: 2.02.2-beta*
*Audience: Shell Enthusiasts, Headless Automated Testers, and Preset Designers.*

Welcome to the command-line documentation for the **Choroboros Dev Panel**. If the standard interface of Choroboros is a simple 6-knob pedal, and the Dev Panel UI is an aircraft cockpit, then the **Interactive Console** is raw access to the flight computer's C++ shell memory.

 *(If you are looking for documentation regarding the visual graphs, mapping structures, and inspector panes, please see `DEV_PANEL_MANUAL.md`).*

---

## Table of Contents
1.  **Architecture & Parsing Mechanics**
    *   1.1 The Lexer Pipeline
    *   1.2 Command Verification & Aliasing
    *   1.3 Fuzzy Path Execution
2.  **State Transactions (Macro Limits)**
    *   2.1 `engine` & `hq` hot swapping
    *   2.2 Bypassing UI Visuals (`view`)
3.  **Property Tuning Commands**
    *   3.1 The `set`, `add`, and `sub` triad
    *   3.2 `toggle` boundaries
    *   3.3 Background Automations (`sweep`)
    *   3.4 UI Locking
4.  **Deep Introspection Utilities**
    *   4.1 Readout parsing (`get`, `list`, `search`)
    *   4.2 `stats`: The Telemetry Matrix
    *   4.3 Visual `watch` HUD pinning
    *   4.4 The `dump` and `diff factory` engines
5.  **History Transactions**
    *   5.1 Atomic `undo` / `redo`
    *   5.2 Command `history` memory buffer
6.  **I/O Scripting (`.choroscript`)**
    *   6.1 Serialization: `export script` and `cp json`
    *   6.2 Instantiation: `import script` and `save defaults`
7.  **The Interactive Tutorial Engine**
    *   7.1 Running sequential HUD layers
    *   7.2 Granular Topic Indexes
8.  **The Master Index: All Parameter Slugs**
    *   8.1 Globals
    *   8.2 Engine Specific Architectures (BBD, Tape, Orbit, Fast-Fourier Transforms)

---

## 1. Architecture & Parsing Mechanics

The Interactive Console sits at the literal bottom of the `Validation` tab window. You can focus it by clicking the black text box directly above the parameter slug definitions. 

### 1.1 The Lexer Pipeline
The interactive console evaluates commands space-separated inputs. It relies heavily on standard parsing: 
1. The user presses `Enter`.
2. The UI pushes the input string to the central C++ message thread.
3. The parser tokenizes the string by whitespaces (`" "`, `"\t"`).
4. The FIRST argument (`tokens[0]`) is the **Action**.
5. Following arguments are interpreted based on the designated Action signature.

### 1.2 Command Verification & Aliasing
Before parsing Actions, the system runs an Alias check up to 8 levels deep.
If you type `alias wipe reset all`, the system maps the string `wipe` to the execution queue `reset all`.
To protect the app from recursive crash loops (e.g. `alias wipe destroy`, `alias destroy wipe`), the parser tracks visited aliases and instantly aborts execution if a loop detects it is consuming its own tail.

### 1.3 Slugification & Fuzzy Matching
Every single text-slider and parameter toggle in the Dev Panel memory block has a "Target Slug."
The C++ system strips the human-readable text `Black HQ Tap2 Delay Offset Base (ms)` through a regex filter converting spaces to underscores and dropping units, generating `black_hq_tap2_delay_offset_base`.
When you target this slider, you must use this exact syntax. However, the system utilizes fuzzy memory matching; if you misspell a slug slightly, or only type part of the string, it will scan through all 90 parameters and attempt to find a single valid conclusion before rejecting the operation as ambiguous.

---

## 2. State Transactions (Macro Limits)

These tools manipulate the bounds of the global plugin algorithm.

### 2.1 Engine & HQ
`engine <color>`
**Parameters:** `green`, `blue`, `red`, `purple`, `black`.
Instantly purges the running chorus iteration from memory, calculates parameter crossfades to prevent auditory pops, and loads the entire DSP memory structure of the new array.
*   *Example:* `engine blue` -> Swaps from the default green bloom to the Blue Focus filter algorithm.

`hq <on/off>`
**Parameters:** `on`, `off`, `true`, `false`, `1`, `0`.
Dictates the internal precision. In most cases, this drastically alters the backend algorithm.
*   *Example:* `hq on` while in `engine red` removes the Bucket-Brigade algorithm entirely and mounts the magnetic Tape module.

### 2.2 Routing
`bypass <on/off>`
Disengages the plugin instantly. Perfect for A/B critical listening during mix sessions where latency matching is imperative to preventing perception bias.

`solo <node>`
**Parameters:** String routing destinations (e.g. `bbd_left`, `tape_input`, `black_tap2_mix`).
Useful for DSP debugging. Cuts audio to all matrices except the specific requested node. Type `unsolo` to revert.

`view <tab_name>`
Forces the UI to instantly render a specific graphical section. Useful for macro building. (e.g. `view modulation`, `view validation`, `view tape`).

---

## 3. Property Tuning Commands

These functions manipulate exact internal memory values of defined parameters.

### 3.1 The Value Triad (`set`, `add`, `sub`)
`set <slug> <value>`
Bypasses the UI completely, ignoring whatever limits are arbitrarily set by the `Min` and `Max` GUI boundaries in the Left Inspector. It forces the true float directly into the AudioProcessor state.
*   *Example:* `set pre_emphasis_gain 24.5` will push the high-end EQ transient punch to absurd levels.

`add <slug> <value>` / `sub <slug> <value>`
Relative mathematical shifting. Reads the active value dynamically and performs standard addition or subtraction.
*   *Example:* If `lpf_cutoff_hz` is currently `1000.0`, running `add lpf_cutoff_hz 500` will smoothly update the active state to `1500.0`.

### 3.2 Toggles Boundaries
`toggle <slug>`
Locates any parameter that operates within a strictly boolean 0 or 1 range, reads it, and instantly flips the bit in memory. 

### 3.3 Background Automations
`sweep <slug> <start_val> <end_val> <duration_in_ms>`
Executes an asynchronous interpolation sequence. Over the specified milliseconds, it pulls the target slider from Start to End while updating the UI GUI 30 times a second to represent the live sweep.
*   *Caveat:* If you manually drag a slider or trigger a new `set` command while a `sweep` is running, the interpolation process aborts instantly to respect physical input hierarchy.

`macro <master_knob_name> <percent(0-100)>`
Simulates a human physically touching the front visual UI. E.g. `macro depth 75` is identical to spinning the Depth knob to 3 o'clock.

### 3.4 UI Locking
`lock <slug>` / `unlock <slug>`
Flags the targeted UI Component as read-only. Standard users will not be able to interact with the slider via mouse inputs. Commands like `set` and `sweep` still perfectly modify the internal values regardless of physical view locks.

---

## 4. Deep Introspection Utilities

These tools do not interact with audio parameters. They read back engine state mapping variables to understand memory context.

### 4.1 Readout Tools
`get <slug>`
Returns the exact float payload that the variable possesses at this static millisecond, but it ALSO prints whatever the previous value was, giving context to exactly how far a parameter moved during the last interpolation sequence.

`search <substring>` 
A heavily optimized fuzzy scan matching against both the slugified name AND the formatted "human readable UI label". It prints out all possible conclusions.

`list <engine_target>`
Accepts `green`, `blue`, `red`, `purple`, `black`, and `globals`. Iterates through the entire registry and outputs the 10-25 variables directly impacting the currently requested algorithm.

### 4.2 Telemetry Engine
`stats`
A brutal memory check mapping plugin performance. Prints:
*   Block Execution Nanosecond durations.
*   Engine Swap counts (Proving memory persistence logic).
*   Live Thread-Lock statuses.

### 4.3 Visual Overlays
`watch <slug>`
Takes a variable and pins it to a real-time HUD rendering at the very top vertical pixels of the console window. Up to 6 widgets can be concurrently mounted to visually diagnose sweeping interpolation functions in real-time. Use `unwatch <slug>` to rip it off the HUD.

### 4.4 The State Comparators
`dump <engine_color>`
Spits the raw values of all active and unactive engine parameters to the console window at once.

`diff factory`
Takes a memory snapshot of your active plugin, instantiates an invisible duplicate processor loaded with factory C++ defaults on a secondary thread, cross-references them, and prints ONLY the explicit parameters you have altered. Crucial for understanding what caused an un-intended artifact.

`reset <slug>`
Using the same secondary thread check, maps the exact payload of the factory algorithm into your selected widget payload variables, immediately restoring the parameter. Or, use `reset all` to completely wipe your session configuration.

---

## 5. History Transactions

Every `set`, `add`, or UI drag triggers an `Atomic Change Gesture`. The console logs these inside a massive 256-step array structure.

### 5.1 The `undo` Stack
`undo`
Reverts your plugin strictly back to the state of the active payload immediately proceeding the last `beginChangeGesture()` flag hit.

`undo <integer>`
A multi-step memory wipe. Iterates deeply through the stack structure, resetting the VST parameters multiple times. (e.g. `undo 5` removes the last 5 operations done). 
You may also trigger `redo <integer>`. 

### 5.2 Command Visualizer
`history`
Outputs the memory stack limits. It specifically differentiates between "Console Text String submissions" and "Visual UI Drag Drop gestures" with accurate atomic timestamps.

---

## 6. I/O Scripting (`.choroscript`)

Because the console acts as an integrated CLI, it makes perfect sense to export complex automation sweeps into text-readable documents.

### 6.1 State Serialization
`export script`
The developer payload generator. It walks through all dynamically scoped properties in your plugin, strips out defaults, and builds a massive string array of commands (e.g. `set bbd_depth 12.0 \n engine red \n`).
It forcefully injects this output into your OS Clipboard. You can paste this text directly into an email or Reddit comment to share custom choruses.

`cp json` 
An alternative to `export script` focused purely on nested mapping variable architectures instead of CLI lexer texts. Perfect for deep data parsing outside of the application.

### 6.2 State Instantiation
`import script`
When triggered, a standard OS File Dialog appears. You can select any standard `.txt` or `.choroscript` file containing new-line separated CLI commands. The Console will lock the parser and run every line at hyper-speed, completely rebuilding the DSP and UI arrays sequentially based on the script rules.

`save defaults`
A hyper-specific alias of `cp json` that writes the JSON payload directly into `startup_prefs.xml`, replacing the application's base definitions upon boot logic.

---

## 7. The Interactive Tutorial Engine

The console is connected heavily to the UI components. Typing `tutorial` hooks into the top-level View engine and launches the first of 6 hard-coded guided lesson maps.

### 7.1 Map Behaviors
When running, the HUD automatically invokes `view <tab>` commands, draws vector borders around specific UI elements, forces specific `engine` variables to clarify DSP context, and waits for user interaction before proceeding to the next step.

*   `tutorial next`: Moves to the next component highlight block.
*   `tutorial next section`: Bypasses granular analysis to hit macro level summaries.
*   `tutorial skip`: Forcefully terminates the active script logic, dumping the user back into unrestricted dev panel manipulation.

### 7.2 The Topic Index
You can jump directly into specific lesson maps by supplying the correct path variables:
1. `tutorial bbd`: Bucket-Brigade Limits & Filtering Constraints.
2. `tutorial tape`: Mechanical wow/flutter offsets on Magnetic loops.
3. `tutorial phase`: Dual LFO independent correlation structures.
4. `tutorial bimodulation`: Asynchronous polynomial rendering on the Purple engine.
5. `tutorial saturation`: Audio wave-limiting by hard and soft analog clipping architectures on Tone tabs.
6. `tutorial envelope`: The Green transient follower equations and decay routing models.

---

## 8. The Master Index: All Parameter Slugs

The following lists are completely comprehensive as of `v2.02.2-beta`. Use these exact string titles to affect parameter tuning via console arrays.

### 8.1 Base Globals 
These impact multi-variable smoothing and core macro routing interpolations.
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

### 8.2 Green Variables (Classic Bloom) 
* `green_bloom_exponent`
* `green_bloom_depth_scale`
* `green_bloom_centre_offset_ms`
* `green_bloom_cutoff_max_hz`
* `green_bloom_cutoff_min_hz`
* `green_bloom_wet_blend`
* `green_bloom_gain`

### 8.3 Blue Variables (Modern Focus) 
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

### 8.4 Red Variables (BBD / Analog Tape) 
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

### 8.5 Purple Variables (Experimental Phase Warp / Orbit) 
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

### 8.6 Black Variables (Linear / Intensity Ensemble) 
* `black_nq_depth_base`
* `black_nq_depth_scale`
* `black_nq_delay_glide_ms`
* `black_hq_tap2_mix_base`
* `black_hq_tap2_mix_scale`
* `black_hq_second_tap_depth_base`
* `black_hq_second_tap_depth_scale`
* `black_hq_second_tap_delay_offset_base`
* `black_hq_second_tap_delay_offset_scale`

### 8.7 Dev Panel Environment Settings 
These settings define the accessibility options and rendering layouts bounding the framework logic inside the Dev Panel proper.
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

*(End of Interactive Console API Manual)*
