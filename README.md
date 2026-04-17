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

1. **Classic (Green NQ)** — R=0.65 Hz, D=21%, O=33°, W=150%, M=50%, C=16%
2. **Vintage (Red HQ)** — R=0.62 Hz, D=21%, O=56°, W=150%, M=50%, C=50%
3. **Modern (Blue HQ)** — R=0.26 Hz, D=53%, O=59°, W=100%, M=50%, C=41%
4. **Psychedelic (Purple NQ)** — R=0.12 Hz, D=52%, O=52°, W=200%, M=69%, C=13%
5. **Core (Black HQ)** — R=0.8 Hz, D=35%, O=41°, W=159%, M=50%, C=28%
6. **Duck (Purple HQ)** — R=10 Hz, D=14%, O=50°, W=50%, M=100%, C=10%
7. **Ouroboros (Blue HQ)** — R=2.0 Hz, D=11%, O=33°, W=33%, M=100%, C=65%

Per-engine parameter memory: switching engines via dropdown restores your last values for that engine.

### Dev Panel (Power Users)

Built-in diagnostic and tuning suite. Click the **DEV** icon in the top-right drawer to open. Features include:
- **Interactive Console & Tutorials:** Type `help` to see 30+ power commands or `tutorial` for interactive, guided DSP walkthroughs.
- **Deep Introspection:** Exposes parameter mapping, DSP internals per engine, and live readout telemetry.
- **UI Customization:** A dedicated Settings panel allows for granular control over typography, themes, and accessibility options (color-vision assistance, reduced motion, hit targets).
- **Validation:** Live DSP trace matrices to confirm signal-flow integrity.

Intended for educators, sound designers, and power users who want to see under the hood.

## Installation

**Downloads (Choroboros Beta v2.05):**

Beta builds are distributed via GitHub Actions artifacts. Go to the [Actions tab](https://github.com/EsotericShadow/choroboros-open-source/actions), open the latest passing Build run, and download the artifact for your platform. Each artifact contains a `.zip` and a `.zip.sha256` checksum file.

| Platform | Artifact name |
|----------|--------------|
| macOS Universal (Intel + Apple Silicon) | `Choroboros-Beta-macOS-Universal` |
| Linux x64 | `Choroboros-Beta-linux-x64` |
| Windows x64 | `Choroboros-Beta-windows-x64` |
| Windows x86 (legacy) | `Choroboros-Beta-windows-x86` |

### macOS — install script (recommended)
The macOS zip includes `install.sh` which handles copying and Gatekeeper quarantine removal automatically:
```bash
cd ~/Downloads/<unzipped-Choroboros-Beta-package>
bash install.sh
```

### macOS — manual VST3
Copy `Choroboros Beta.vst3` to:
- `~/Library/Audio/Plug-Ins/VST3/` (user-specific, recommended)
- `/Library/Audio/Plug-Ins/VST3/` (system-wide)

### macOS — manual AU
Copy `Choroboros Beta.component` to:
- `~/Library/Audio/Plug-Ins/Components/` (user-specific, recommended)
- `/Library/Audio/Plug-Ins/Components/` (system-wide)

### macOS — Standalone
Copy `Choroboros Beta.app` to `/Applications/` or anywhere you prefer.

### Windows — VST3
Extract the zip. Copy `Choroboros Beta.vst3` to:
- `C:\Program Files\Common Files\VST3\` (system-wide)
- `%LOCALAPPDATA%\Programs\Common\VST3\` (user-specific)

### Windows — Standalone
Extract the zip. Run `Choroboros Beta.exe` directly or place it anywhere you prefer.

### Linux — VST3
Extract the zip. Copy `Choroboros Beta.vst3` to:
- `~/.vst3/` (user-specific, recommended)
- `/usr/lib/vst3/` (system-wide, requires sudo)

### Linux — Standalone
Extract the zip. Make `Choroboros Beta` executable and run it:
```bash
chmod +x "Choroboros Beta"
./"Choroboros Beta"
```

After installation, rescan plugins in your DAW.

**Beta site:** [https://choro-beta-site.vercel.app/](https://choro-beta-site.vercel.app/) — join the beta, roadmap, whitepaper, feedback.

**Beta testers:** See [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md) for known issues and how to report feedback.

## System Requirements

- **macOS:** 10.13 High Sierra or later (Intel or Apple Silicon)
- **Windows:** Windows 10 or later (x64 or x86)
- **DAW:** Any DAW that supports VST3 or AU plugins

> **Note:** Macs that cannot run macOS 10.13 (such as Mac Pro Early 2009 and older) are not supported. The JUCE 8 framework requires 10.13 as a minimum deployment target. AU validation will fail on unsupported systems.

## Maintainers: factory defaults JSON

The bundled factory sheet exists as **four copies** that must stay **byte-identical** (`Assets/defaults_factory_mac.json` plus the three platform BinaryData inputs). Before you merge factory changes or cut a release, run `./scripts/verify_factory_json_sync.sh` from the repo root. Full procedure: [docs/FACTORY_DEFAULTS_JSON_SYNC.md](docs/FACTORY_DEFAULTS_JSON_SYNC.md).

## Technical Details

- **Version:** v2.05
- **Company:** Kaizen DSP
- **Location:** British Columbia, Canada
- **Framework:** JUCE 8.0.12
- **Sample Rate:** Up to 192 kHz (all DSP is sample-rate invariant as of v2.05)
- **Formats:** VST3, AU, Standalone (macOS); VST3, Standalone (Windows)
- **macOS:** Universal binary (arm64 + x86_64)
- **Windows:** x64 primary, x86-compat

## License

Licensing depends on **what you are using**: the **public source tree** in this repository, or an **official Choroboros Beta binary** from Kaizen (installer, signed build, or other channel Kaizen provides). This section is a plain-language summary; the **End User License Agreement** (`EULA.md`) governs official beta binaries, and **GPLv3** governs the published source as described below. *This is not legal advice.*

### Source code in this repository (GPLv3)

The source code in **https://github.com/EsotericShadow/choroboros-open-source** is free software licensed under the **GNU General Public License version 3 (GPLv3)** (or, at your option, any later version). See `LICENSE` and `COPYING` for the full license text.

You may run, study, modify, and redistribute **your own builds** made from that source under the terms of GPLv3.

Copyright (C) 2026 Kaizen Strategic AI Inc.

### Official Choroboros Beta binaries (EULA)

**Pre-built beta plug-ins** that Kaizen Strategic AI Inc. / Kaizen DSP distributes in compiled form (for example a **signed / notarized macOS installer**, or other official beta download) are **not** offered to you under GPLv3. Your use of those builds is under the **End User License Agreement** in `EULA.md` (also bundled in the plug-in where applicable). That agreement includes proprietary terms, beta disclaimers, and restrictions that do **not** apply in the same way to GPLv3-only self-builds from this repo.

Kaizen builds Choroboros using the **JUCE** framework under a **commercial JUCE license** for this product, in addition to its own rights in the Software Product. For JUCE’s own terms, see https://juce.com/legal/juce-8-licence/

### Third-party components

This software uses the JUCE framework and other components that may be subject to their own license terms; see repository notices and bundled documentation where shipped.

## Support & Contact

For issues, questions, or licensing inquiries:
- **Email:** info@kaizenstrategic.ai
- **Company:** Kaizen DSP
- **Location:** British Columbia, Canada

## Installation Troubleshooting

- **Plugins not appearing in DAW:** Make sure you've copied the plugin to the correct location and rescanned plugins in your DAW. Some DAWs require a full restart.
- **macOS Gatekeeper:** Prefer a **signed, notarized** `.pkg` from releases when available. For unsigned zips or local builds, use `bash install.sh` or `xattr -cr` as in [KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md).
- **AU validation failure:** Ensure your Mac is running macOS 10.13 or later. If validation fails, try `killall -9 AudioComponentRegistrar` then rescan in Logic.
- **Permission Issues:** For system-wide installation (`/Library/` on macOS, `C:\Program Files\` on Windows), you may need administrator privileges. User-specific installation (`~/Library/` on macOS) is recommended.
