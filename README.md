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
- **HQ:** High-quality mode toggle

### Presets

1. **Classic (Green)** — R=1.2 Hz, D=21%, O=33°, W=150%, M=50%, C=16%
2. **Modern (Blue)** — R=0.26 Hz, D=53%, O=59°, W=100%, M=50%, C=41%
3. **Vintage (Red)** — R=0.62 Hz, D=21%, O=56°, W=150%, M=50%, C=50%
4. **Psychedelic (Purple)** — R=0.12 Hz, D=52%, O=52°, W=200%, M=69%, C=13%
5. **Core (Black)** — R=1.2 Hz, D=35%, O=41°, W=159%, M=50%, C=28%
6. **Duck** — Purple HQ, fast modulation
7. **Ouroboros** — Blue HQ, medium modulation

Per-engine parameter memory: switching engines via dropdown restores your last values for that engine.

## Installation

Download the macOS Universal package (`Choroboros-v2.02-beta-macOS-Universal.zip`) from Releases, or use `install.sh` from the distribution.

### VST3
Copy `Choroboros.vst3` to:
- `/Library/Audio/Plug-Ins/VST3/` (system-wide)
- `~/Library/Audio/Plug-Ins/VST3/` (user-specific)

### AU
Copy `Choroboros.component` to:
- `/Library/Audio/Plug-Ins/Components/` (system-wide)
- `~/Library/Audio/Plug-Ins/Components/` (user-specific)

### AAX (Pro Tools)
Copy `Choroboros.aaxplugin` to:
- `~/Library/Application Support/Avid/Audio/Plug-Ins/` (user)
- `/Library/Application Support/Avid/Audio/Plug-Ins/` (system)

### Standalone
Copy `Choroboros.app` to `/Applications/` or any location you prefer.

After installation, rescan plugins in your DAW.

**Beta testers:** See [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md) for known issues and how to report feedback.

## System Requirements

- **macOS:** 10.13 or later
- **DAW:** Any DAW that supports VST3 or AU plugins
- **CPU:** Intel or Apple Silicon

## Technical Details

- **Version:** 2.02-beta
- **Company:** Kaizen Strategic AI Inc. (DBA: Green DSP)
- **Location:** British Columbia, Canada
- **Framework:** JUCE 8.0.12
- **Sample Rate:** Up to 192 kHz
- **Formats:** VST3, AU, AAX, Standalone
- **macOS:** Universal binary (arm64 + x86_64)

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
- **Company:** Kaizen Strategic AI Inc.
- **DBA:** Green DSP
- **Location:** British Columbia, Canada

## Installation Troubleshooting

- **Plugins not appearing in DAW:** Make sure you've copied the plugin to the correct location and rescanned plugins in your DAW. Some DAWs require a full restart.
- **macOS Security:** If macOS blocks the plugin, go to System Preferences > Security & Privacy and allow the plugin.
- **Permission Issues:** For system-wide installation (`/Library/`), you may need administrator privileges. User-specific installation (`~/Library/`) is recommended.
