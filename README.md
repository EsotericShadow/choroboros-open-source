# Choroboros

**A chorus that eats its own tail — Five colors, ten algorithms**

Choroboros is a multi-engine chorus plugin with five distinct engines, each offering two algorithms (Normal/HQ). Each engine has its own color semantics: Green adds bloom (thickness/damping), Blue adds focus (clarity/presence), Red NQ adds saturation, Red HQ adds tape tone, Purple warps phase, and Black modulates ensemble spread.

## Features

### Five Engine Colors

- **🟢 Green (Classic):** Bloom — thickness and gentle vintage softening (no saturation)
  - Normal: 3rd order Lagrange
  - HQ: 5th order Lagrange

- **🔵 Blue (Modern):** Focus — clarity and definition (no saturation)
  - Normal: Cubic interpolation
  - HQ: Thiran allpass interpolation

- **🔴 Red (Vintage):** Analog character
  - Normal: BBD (Bucket Brigade Delay) emulation with 5th-order cascade filtering — saturation only
  - HQ: Tape-style chorus — tone + drive

- **🟣 Purple (Experimental):** Warp and orbit — psychedelic phase modulation
  - Normal: Phase-Warped Chorus (non-uniform phase modulation)
  - HQ: Orbit Chorus (2D rotating modulation)

- **⬛ Black (Core/Linear):** Modulation intensity and ensemble spread
  - Normal: Linear interpolation
  - HQ: Linear Ensemble (multi-voice)

### Parameters

- **Rate:** LFO speed (0.01–20 Hz); right-click for musical quantize (Straight/Triplet/Dotted, cap 20 Hz)
- **Depth:** Modulation depth (0–100%, engine-specific scaling)
- **Offset:** LFO phase offset (0–180°)
- **Width:** Stereo width (0–200%)
- **Color:** Engine-specific (Bloom / Focus / Saturation / Tape / Warp / Mod intensity)
- **Mix:** Dry/wet mix (0–100%)
- **HQ:** Algorithm mode toggle - switches each engine to an alternate DSP algorithm. HQ modes use more sophisticated processing (ensemble, orbit, tape, higher-order interpolation) which typically produces wider stereo imaging and richer character, not just higher fidelity.

### Presets

1. **Classic (Green)** — R=0.65 Hz, D=21%, O=33°, W=150%, M=50%, C=16%
2. **Modern (Blue)** — R=0.26 Hz, D=53%, O=59°, W=100%, M=50%, C=41%
3. **Vintage (Red)** — R=0.62 Hz, D=21%, O=56°, W=150%, M=50%, C=50%
4. **Psychedelic (Purple)** — R=0.12 Hz, D=52%, O=52°, W=200%, M=69%, C=13%
5. **Core (Black)** — R=0.8 Hz, D=35%, O=41°, W=159%, M=50%, C=28%
6. **Duck** — Purple HQ, fast modulation
7. **Ouroboros** — Blue HQ, medium modulation

Per-engine parameter memory: switching engines via dropdown restores your last values for that engine.

### Dev Panel (Power Users)

Built-in diagnostic and tuning suite. Click the **DEV** button (top-left) to open. Features include:
- **Interactive Console & Tutorials:** Type `help` to see 30+ power commands or `tutorial` for interactive, guided DSP walkthroughs.
- **Deep Introspection:** Exposes parameter mapping, DSP internals per engine, and live readout telemetry.
- **UI Customization:** A dedicated Settings panel allows for granular control over typography, themes, and accessibility options (color-vision assistance, reduced motion, hit targets).
- **Validation:** Live DSP trace matrices to confirm signal-flow integrity.

Intended for educators, sound designers, and power users who want to see under the hood.

## Installation

**Downloads (v2.03-beta):**

| Platform | File | Link |
|----------|------|------|
| macOS Universal | `Choroboros-v2.03-beta-macOS-Universal.zip` | [Download](https://github.com/EsotericShadow/choroboros-open-source/releases/download/v2.03-beta/Choroboros-v2.03-beta-macOS-Universal.zip) |
| Windows x64 | `Choroboros-v2.03-beta-Windows-x64.zip` | [Download](https://github.com/EsotericShadow/choroboros-open-source/releases/download/v2.03-beta/Choroboros-v2.03-beta-Windows-x64.zip) |
| Windows x86 | `Choroboros-v2.03-beta-Windows-x86-compat.zip` | [Download](https://github.com/EsotericShadow/choroboros-open-source/releases/download/v2.03-beta/Choroboros-v2.03-beta-Windows-x86-compat.zip) |

SHA256 checksums included for all builds. Or use `install.sh` from the distribution (macOS).

### macOS — VST3
Copy `Choroboros.vst3` to:
- `/Library/Audio/Plug-Ins/VST3/` (system-wide)
- `~/Library/Audio/Plug-Ins/VST3/` (user-specific)

### macOS — AU
Copy `Choroboros.component` to:
- `/Library/Audio/Plug-Ins/Components/` (system-wide)
- `~/Library/Audio/Plug-Ins/Components/` (user-specific)

### macOS — AAX (Pro Tools)
Copy `Choroboros.aaxplugin` to:
- `~/Library/Application Support/Avid/Audio/Plug-Ins/` (user)
- `/Library/Application Support/Avid/Audio/Plug-Ins/` (system)

### macOS — Standalone
Copy `Choroboros.app` to `/Applications/` or any location you prefer.

### Windows — VST3
Extract the zip and copy `VST3/Choroboros.vst3` to:
- `C:\Program Files\Common Files\VST3\` (system-wide)
- `%LOCALAPPDATA%\Programs\Common\VST3\` (user-specific)

### Windows — Standalone
Extract the zip and run `Standalone/Choroboros.exe` or place it in your preferred location.

After installation, rescan plugins in your DAW.

**Beta site:** [https://choro-beta-site.vercel.app/](https://choro-beta-site.vercel.app/) — join the beta, roadmap, whitepaper, feedback.

**Beta testers:** See [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md) for known issues and how to report feedback.

## System Requirements

- **macOS:** 10.13 High Sierra or later (Intel or Apple Silicon)
- **Windows:** Windows 10 or later (x64 or x86)
- **DAW:** Any DAW that supports VST3 or AU plugins

> **Note:** Macs that cannot run macOS 10.13 (such as Mac Pro Early 2009 and older) are not supported. The JUCE 8 framework requires 10.13 as a minimum deployment target. AU validation will fail on unsupported systems.

## Technical Details

- **Version:** 2.04
- **Company:** Kaizen DSP
- **Location:** British Columbia, Canada
- **Framework:** JUCE 8.0.12
- **Sample Rate:** Up to 192 kHz
- **Formats:** VST3, AU, AAX, Standalone (macOS); VST3, Standalone (Windows)
- **macOS:** Universal binary (arm64 + x86_64)
- **Windows:** x64 primary, x86-compat

## License

**Choroboros is free software licensed under the GNU General Public License version 3 (GPLv3).**

Copyright (C) 2026 Kaizen Strategic AI Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

### Source Code

The complete source code for Choroboros is available at:
**https://github.com/EsotericShadow/choroboros-open-source**

You are free to view, modify, and redistribute the source code under the terms of the GPLv3 license.

### Third-Party Components

This software uses the JUCE framework, which is dual-licensed under:
- GNU Affero General Public License v3 (AGPLv3), or
- Commercial license from Raw Material Software Limited

For more information about JUCE licensing, visit: https://juce.com/legal/juce-8-licence/

## Support & Contact

For issues, questions, or licensing inquiries:
- **Email:** Greenalderson@gmail.com
- **Company:** Kaizen DSP
- **Location:** British Columbia, Canada

## Installation Troubleshooting

- **Plugins not appearing in DAW:** Make sure you've copied the plugin to the correct location and rescanned plugins in your DAW. Some DAWs require a full restart.
- **macOS Gatekeeper (beta builds):** The beta is not yet code-signed. If macOS blocks the plugin, use the install script (`bash install.sh`) which handles quarantine removal automatically. Or manually run `xattr -cr` on each plugin file — see [KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md) for detailed steps.
- **AU validation failure:** Ensure your Mac is running macOS 10.13 or later. If validation fails, try `killall -9 AudioComponentRegistrar` then rescan in Logic.
- **Permission Issues:** For system-wide installation (`/Library/` on macOS, `C:\Program Files\` on Windows), you may need administrator privileges. User-specific installation (`~/Library/` on macOS) is recommended.
