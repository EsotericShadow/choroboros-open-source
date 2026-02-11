# Choroboros Plugin Packaging Guide

## Overview
Choroboros is a multi-engine chorus plugin with four distinct engines and eight algorithms total.

**Version:** 1.0.0  
**Company:** Kaizen Strategic AI  
**Formats:** VST3, AU, Standalone

## Build Artifacts

After building, the following artifacts are created in `Build/Choroboros_artefacts/`:

- **VST3:** `VST3/Choroboros.vst3`
- **AU:** `AU/Choroboros.component`
- **Standalone:** `Standalone/Choroboros.app`

## Pre-Packaging Checklist

### ✅ Code & Assets
- [x] All four engine colors implemented (Green, Blue, Red, Purple)
- [x] All eight algorithms working (2 per engine)
- [x] All UI assets included (backgrounds, knobs, sliders, mix knobs)
- [x] Presets configured (Classic, Vintage, Rich, Psychedelic, Duck, Ouroboros)
- [x] Mix knob functionality working
- [x] Engine-specific parameter mappings (Purple depth scaling)

### ✅ Build Configuration
- [x] Version number set (1.0.0)
- [x] Company name set (Kaizen Strategic AI)
- [x] Bundle ID set (com.kaizenstrategicai.Choroboros)
- [x] Description updated (Four colors, eight algorithms)
- [x] All formats building (VST3, AU, Standalone)

### ✅ Testing
- [ ] Tested in Reaper (VST3)
- [ ] Tested in Logic/GarageBand (AU)
- [ ] Tested Standalone app
- [ ] All engines switch correctly
- [ ] All presets load correctly
- [ ] Parameter automation works
- [ ] No audio artifacts (crackle, zippering)
- [ ] UI responds correctly to all interactions

## Packaging Steps

### 1. Clean Build
```bash
cd /Users/main/Desktop/green_chorus
rm -rf Build
cmake -B Build -DCMAKE_BUILD_TYPE=Release
cmake --build Build --config Release
```

### 2. Verify Build Artifacts
```bash
# Check all formats exist
ls -lh Build/Choroboros_artefacts/VST3/Choroboros.vst3
ls -lh Build/Choroboros_artefacts/AU/Choroboros.component
ls -lh Build/Choroboros_artefacts/Standalone/Choroboros.app
```

### 3. Code Signing (macOS)
For distribution, you may need to code sign the plugins:

```bash
# Sign VST3
codesign --force --deep --sign "Developer ID Application: Your Name" \
  Build/Choroboros_artefacts/VST3/Choroboros.vst3

# Sign AU
codesign --force --deep --sign "Developer ID Application: Your Name" \
  Build/Choroboros_artefacts/AU/Choroboros.component

# Sign Standalone
codesign --force --deep --sign "Developer ID Application: Your Name" \
  Build/Choroboros_artefacts/Standalone/Choroboros.app
```

### 4. Create Installer Package (Optional)
For macOS, you can create a `.pkg` installer using `pkgbuild` or a tool like Packages.app.

### 5. Create Distribution Archive
```bash
# Create a release folder
mkdir -p Release/Choroboros-v1.0.0

# Copy plugins
cp -R Build/Choroboros_artefacts/VST3 Release/Choroboros-v1.0.0/
cp -R Build/Choroboros_artefacts/AU Release/Choroboros-v1.0.0/
cp -R Build/Choroboros_artefacts/Standalone Release/Choroboros-v1.0.0/

# Add README, license, etc.
# cp README.md Release/Choroboros-v1.0.0/
# cp LICENSE Release/Choroboros-v1.0.0/

# Create archive
cd Release
zip -r Choroboros-v1.0.0-macOS.zip Choroboros-v1.0.0/
```

## Installation Instructions (for end users)

### VST3
1. Copy `Choroboros.vst3` to:
   - `/Library/Audio/Plug-Ins/VST3/` (system-wide)
   - `~/Library/Audio/Plug-Ins/VST3/` (user-specific)
2. Rescan plugins in your DAW

### AU
1. Copy `Choroboros.component` to:
   - `/Library/Audio/Plug-Ins/Components/` (system-wide)
   - `~/Library/Audio/Plug-Ins/Components/` (user-specific)
2. Rescan plugins in your DAW

### Standalone
1. Copy `Choroboros.app` to `/Applications/` or anywhere you prefer
2. Double-click to launch

## Plugin Features

### Engines
- **Green (Classic):** Lagrange 3rd & 5th order interpolation
- **Blue (Modern):** Cubic & Thiran allpass interpolation
- **Red (Vintage):** BBD & Tape emulation
- **Purple (Experimental):** Phase-Warped & Orbit chorus

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

## Technical Details

- **Framework:** JUCE 8.0.12
- **C++ Standard:** C++17
- **Platform:** macOS (Intel & Apple Silicon)
- **Sample Rate:** Up to 192 kHz
- **Buffer Size:** Variable (DAW-dependent)

## Support

For issues or questions, contact: Kaizen Strategic AI
