# Choroboros

**A chorus that eats its own tail - Four colors, eight algorithms**

Choroboros is a multi-engine chorus plugin featuring four distinct engines, each with two unique algorithms. Switch between engines to explore classic, modern, vintage, and experimental chorus sounds.

## Features

### Four Engine Colors

- **🟢 Green (Classic):** Smooth, musical chorus with Lagrange interpolation
  - Normal: 3rd order Lagrange
  - HQ: 5th order Lagrange

- **🔵 Blue (Modern):** Clean, transparent chorus with advanced interpolation
  - Normal: Cubic interpolation
  - HQ: Thiran allpass interpolation

- **🔴 Red (Vintage):** Warm, characterful chorus with analog emulation
  - Normal: BBD (Bucket Brigade Delay) emulation
  - HQ: Tape-style chorus

- **🟣 Purple (Experimental):** Unique, psychedelic chorus algorithms
  - Normal: Phase-Warped Chorus (non-uniform phase modulation)
  - HQ: Orbit Chorus (2D rotating modulation)

### Parameters

- **Rate:** LFO speed (0.01 - 20 Hz)
- **Depth:** Modulation depth (0-100%, engine-specific scaling)
- **Offset:** LFO phase offset (0-180°)
- **Width:** Stereo width (0-200%)
- **Color:** Engine-specific parameter (varies by engine)
- **Mix:** Dry/wet mix (0-100%)
- **HQ:** High-quality mode toggle (varies by engine)

### Presets

1. **Classic** - Green engine, classic chorus sound
2. **Vintage** - Red engine, warm tape-style chorus
3. **Rich** - Blue engine, modern clean chorus
4. **Psychedelic** - Purple engine, experimental warped chorus
5. **Duck** - Purple HQ, fast modulation
6. **Ouroboros** - Blue HQ, medium modulation

## Installation

### VST3
Copy `Choroboros.vst3` to:
- `/Library/Audio/Plug-Ins/VST3/` (system-wide)
- `~/Library/Audio/Plug-Ins/VST3/` (user-specific)

### AU
Copy `Choroboros.component` to:
- `/Library/Audio/Plug-Ins/Components/` (system-wide)
- `~/Library/Audio/Plug-Ins/Components/` (user-specific)

### Standalone
Copy `Choroboros.app` to `/Applications/` or any location you prefer.

After installation, rescan plugins in your DAW.

## System Requirements

- **macOS:** 10.13 or later
- **DAW:** Any DAW that supports VST3 or AU plugins
- **CPU:** Intel or Apple Silicon

## Technical Details

- **Version:** 1.0.1
- **Company:** Kaizen Strategic AI Inc. (DBA: Green DSP)
- **Location:** British Columbia, Canada
- **Framework:** JUCE 8.0.12
- **Sample Rate:** Up to 192 kHz
- **Formats:** VST3, AU, Standalone

## License

Copyright © 2026 Kaizen Strategic AI Inc. All rights reserved.

This software is licensed, not sold. By installing or using Choroboros, you agree to the terms of the End User License Agreement (EULA). The EULA can be viewed from the About dialog within the plugin, or contact us for a copy.

**Proprietary Algorithms:** The Purple engine algorithms (Phase-Warped Chorus and Orbit Chorus) are proprietary intellectual property of Kaizen Strategic AI Inc. and are protected by trade secret law.

## Support & Contact

For issues, questions, or licensing inquiries:
- **Email:** info@kaizenstrategic.ai
- **Company:** Kaizen Strategic AI Inc.
- **DBA:** Green DSP
- **Location:** British Columbia, Canada

## Installation Troubleshooting

- **Plugins not appearing in DAW:** Make sure you've copied the plugin to the correct location and rescanned plugins in your DAW. Some DAWs require a full restart.
- **macOS Security:** If macOS blocks the plugin, go to System Preferences > Security & Privacy and allow the plugin.
- **Permission Issues:** For system-wide installation (`/Library/`), you may need administrator privileges. User-specific installation (`~/Library/`) is recommended.
