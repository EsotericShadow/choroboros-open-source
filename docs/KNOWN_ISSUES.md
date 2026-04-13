# Known Issues

Issues known for Choroboros Beta v2.05. Please report additional issues via the Feedback button.

## Current Test Scope

- **Platform:** macOS (VST3, AU, Standalone) and Windows (VST3, Standalone — x64 primary, x86-compat). Linux not yet available.
- **Dev Panel:** Built-in diagnostic and tuning suite. Click the drawer tab (top-right) to expand the icon bar, then click the **DEV** icon to open. Intended for power users; some controls may be experimental.

## System Requirements

- **macOS:** 10.13 (High Sierra) or later — Intel or Apple Silicon
- **Windows:** Windows 10 or later — x64 or x86

Macs that cannot run macOS 10.13 (such as Mac Pro Early 2009 and older) are **not supported**. The plugin may fail AU validation or crash on load on these systems. This is a limitation of the JUCE 8 framework, which requires macOS 10.13 as a minimum deployment target.

## Audio / DSP

### Fixed in v2.05

- **Sample-rate invariance (all engines):** Five DSP constants were hardcoded as raw pole values tuned at 48 kHz. At other sample rates (44.1k, 96k, 192k) they silently degraded — smoothing speeds changed, crossfade durations shrank, and the Tape varispeed spring tightened. All five are now computed from physical time constants at the consumption site, making behavior identical at any supported sample rate.
- **Thiran broadband fuzz/distortion (Blue HQ):** Per-sample coefficient updates caused DFII-T state-coefficient mismatch transients. Extended crossfade from 1ms to 6ms with state-copy architecture on boundary crossings, reducing THD from 1.47% to 0.14%.
- **Width text input parsing:** Typing "1" into the Width field set it to 100%. Parser now correctly divides by 100 to match the displayed percentage format.
- **3 dB volume dip at 50% wet mix:** Linear dry/wet mixing caused a volume dip when wet signal has phase offset from stereo modulation (which chorus always does). Fixed with equal-power sin/cos crossfade (industry standard). Added per-engine output trim parameter for gain compensation.
- **Zippering artifacts across all 5 engines:** Per-sample interpolation smoothing and coefficient fixes across Green (Lagrange), Blue (Cubic/Thiran), Red (Tape), and Black (Ensemble) cores. Reported by multiple testers as "DSP anomalies" and "scratchy" sound.
- **Three additional freeze-on-close vectors:** Beyond the D3D11 fix in v2.04.1, three more teardown issues resolved: DevPanelWindow HWND cascade under DLL loader lock, themePrewarmThread outliving DLL unload, and FeedbackCollector blocking disk I/O in destructor.
- **Preset browser cycling bug:** Preset index reset during load due to parameter callbacks. Fixed with loadInProgress guard.
- **Text entry "1" = 100%:** Typing "1" into any knob value field was misinterpreted as 100%. Parsers for Depth, Color, Mix, and Width now correctly treat values 1-100 as percentages. Offset parser accepts both "deg" and degree symbol suffixes.
- **Color slider "sounds reversed" after engine switch:** Slider value now re-syncs after skew factor change on engine switch.
- **Timer race condition on shutdown:** Defensive atomic guard so timerCallback() cannot execute after plugin destruction begins.
- **Logic Pro hidden editor leak:** Editor now stops background threads when Logic hides the editor without destroying it.
- **Dialog destruction crashes:** About/Feedback/Help dialogs replaced delete-this with SafePointer + callAsync for safe destruction.
- **Non-ASCII character rendering:** Em dashes, curly quotes, arrows, and math symbols rendered as garbled text in JUCE fonts. Replaced with ASCII equivalents.
- **DPI / HiDPI scaling:** Plugin now responds to host-reported DPI scale factor via setScaleFactor() override. Renders correctly on Retina, 4K, and scaled displays in hosts that support VST3 DPI notification (Reaper, Cubase, Studio One, Bitwig, etc.).

### Fixed in v2.04-dev

- **BBD (Red NQ) phaser-like sweep:** S&H clock images aliased into audio band and swept with LFO. Fixed with first-order hold interpolation (~40 dB alias rejection). Also fixed a1 coefficient sign in the Butterworth cascade filter and raised clock/filter frequency floors.
- **Tape (Red HQ) rate knob unresponsive at high rates:** Undamped phase integrator caused DC drift; LFO smoothing bandwidth was too narrow (fc≈10 Hz). Fixed with damped integrator and widened smoothing (fc≈56 Hz).
- **Thiran (Blue HQ) zippering and noise:** Per-sample 5th-order allpass coefficient recomputation caused DFII-T state transients. Fixed with 32-sample linear coefficient interpolation (~30 dB transient reduction).
- **Gain staging:** Pre-emphasis now applies only to wet path. Legacy full-output compressor replaced with transparent post-sum peak catcher (-2 dB threshold, 2:1 ratio, 4 dB soft knee).

### Fixed in v2.03

- **Red engine (BBD) at Depth 0%:** BBD clock minimum was set too low, causing distortion artifacts.
- **Volume boost at 50% mix:** ~1–1.5 dB added volume when plugin engaged. Fixed with wet-path-only pre-emphasis.

## HQ vs NQ Mode Differences

HQ mode is not just a fidelity toggle — each engine's HQ algorithm is a fundamentally different DSP topology:

- **Green:** NQ uses 3rd-order Lagrange interpolation; HQ uses 5th-order Lagrange (smoother, preserves more HF stereo content)
- **Blue:** NQ uses cubic interpolation; HQ uses 5th-order Thiran allpass (phase-accurate, wider imaging)
- **Red:** NQ emulates a BBD chip (bucket brigade saturation); HQ emulates tape (tone + drive + stereo width)
- **Purple:** NQ uses phase-warped LFO; HQ uses 2D orbital modulation (inherently wider stereo field)
- **Black:** NQ is a single-tap linear delay; HQ is a dual-tap ensemble with independent stereo decorrelation

This means HQ modes will generally sound wider and more spacious than NQ — this is intentional. The Width knob affects both modes equally; the difference comes from the algorithms themselves.

HQ toggle now switches in ~43 ms (18 ms warmup + 25 ms crossfade) and supports single-click toggling.

## UI / UX

- **Knob sensitivity:** Default knob sensitivity may feel too high or too low depending on your mouse/trackpad. Use the Dev Panel to fine-tune sensitivity per-knob.

## Compatibility

- **DAWs tested:** Reaper (macOS + Windows), Logic Pro (macOS), Ableton Live (Windows), FL Studio (Windows), Samplitude (Windows). Additional DAW reports welcome.
- **Studio One / Fender Studio Pro:** Bugs reported in these hosts but not yet investigated — access to these environments is required before issues can be addressed. Fix ETA unknown. If you're running either host, in-plugin feedback or email is especially valuable.
- **Ardour 8.10 (Windows):** One report of VST3 crash on load. Appears to be an Ardour VST3 hosting issue — other plugins also crash for this user.
- **Sample rates:** Supports up to 192 kHz. As of v2.05, all DSP is sample-rate invariant — smoothing, crossfades, and modulation behave identically at 44.1k, 48k, 96k, and 192k. Report any issues at non-standard sample rates.

## macOS Gatekeeper (signed installer vs zip beta)

**Preferred:** Install from the **signed, notarized** `.pkg` when we ship it (see GitHub Releases). After notarization, Gatekeeper should allow a normal double-click install without workarounds.

**If you only have an unsigned zip** (older betas or local builds): macOS may block the plugin or show security warnings.

**To install an unsigned zip despite Gatekeeper:**

1. **Recommended — use the install script:** Open Terminal, navigate to the unzipped folder, and run:
   ```
   bash install.sh
   ```
   The script copies plugin files and removes quarantine attributes automatically.

2. **Manual quarantine removal:** If you installed manually and your DAW won't load the plugin, run:
   ```
   xattr -cr ~/Library/Audio/Plug-Ins/VST3/Choroboros\ Beta.vst3
   xattr -cr ~/Library/Audio/Plug-Ins/Components/Choroboros\ Beta.component
   xattr -cr /Applications/Choroboros\ Beta.app
   ```

3. **"Open Anyway" in System Settings:** Go to System Settings → Privacy & Security, scroll down, and click "Open Anyway" next to the Choroboros Beta warning. This option is **unreliable on recent macOS** for unsigned code; prefer the `.pkg` or `xattr` steps above.

Maintainers: full macOS pipeline is `./scripts/release_macos_signed_installer.sh` (universal build → sign bundles → `.pkg` → notarize → staple). See `installer/installer_config.sh` for version alignment and optional env overrides.

## Reporting Issues

Use the **Feedback** button in the plugin to submit issues via our Google Form. Usage statistics can be saved to a file and pasted into the form.
