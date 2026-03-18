# Known Issues

Issues known for Choroboros v2.04-dev. Please report additional issues via the Feedback button.

## Current Test Scope

- **Platform:** macOS (VST3, AU, AAX, Standalone) and Windows (VST3, Standalone — x64 primary, x86-compat). Linux not yet available.
- **Dev Panel:** Built-in diagnostic and tuning suite. Click the drawer tab (top-right) to expand the icon bar, then click the **DEV** icon to open. Intended for power users; some controls may be experimental.

## System Requirements

- **macOS:** 10.13 (High Sierra) or later — Intel or Apple Silicon
- **Windows:** Windows 10 or later — x64 or x86

Macs that cannot run macOS 10.13 (such as Mac Pro Early 2009 and older) are **not supported**. The plugin may fail AU validation or crash on load on these systems. This is a limitation of the JUCE 8 framework, which requires macOS 10.13 as a minimum deployment target.

## Audio / DSP

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
- **Ardour 8.10 (Windows):** One report of VST3 crash on load. Appears to be an Ardour VST3 hosting issue — other plugins also crash for this user.
- **Sample rates:** Supports up to 192 kHz. Report any issues at extreme sample rates.

## macOS Gatekeeper (Unsigned Beta)

Choroboros is not yet code-signed or notarized (pending Apple Developer enrollment). macOS will show a security warning when you first open the plugin or installer.

**To install despite Gatekeeper warnings:**

1. **Recommended — use the install script:** Open Terminal, navigate to the unzipped folder, and run:
   ```
   bash install.sh
   ```
   The script copies plugin files and removes quarantine attributes automatically.

2. **Manual quarantine removal:** If you installed manually and your DAW won't load the plugin, run:
   ```
   xattr -cr ~/Library/Audio/Plug-Ins/VST3/Choroboros.vst3
   xattr -cr ~/Library/Audio/Plug-Ins/Components/Choroboros.component
   xattr -cr /Applications/Choroboros.app
   ```

3. **"Open Anyway" in System Settings:** Go to System Settings → Privacy & Security, scroll down, and click "Open Anyway" next to the Choroboros warning. This button only appears **after** you've attempted to open the blocked file.

4. **If "Open Anyway" doesn't appear:** Some macOS versions (especially Ventura+) hide this option. Use the Terminal xattr method above instead.

Code signing and notarization will be added in a future release.

## Reporting Issues

Use the **Feedback** button in the plugin to submit issues via our Google Form. Usage statistics can be saved to a file and pasted into the form.
