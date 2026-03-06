# Technical Specs Corrections for Beta Site

Use these corrections to update the Technical Specs section at [choro-beta-site.vercel.app](https://choro-beta-site.vercel.app/). Verified against the codebase (v2.03-beta).

---

## 1. Extra features — Rate Sync

**Current (incorrect):**
```
Rate Sync: Right-click Rate for tempo-synced divisions (straight, triplet, dotted, swing). Uses host BPM.
```

**Replace with:**
```
Rate Sync: Right-click Rate for tempo-synced divisions (straight, triplet, dotted). Uses host BPM.
```

**Reason:** Swing is not implemented. The menu has Straight, Triplet, and Dotted subdivisions only.

---

## 2. Formats & platform

**Current (incorrect):**
```
VST3, AU, Standalone · JUCE 8 · macOS (Intel + Apple Silicon) only · Windows coming soon
```

**Replace with:**
```
VST3, AU, AAX, Standalone · JUCE 8 · macOS (Intel + Apple Silicon) · Windows (x64 + x86-compat)
```

**Reason:** AAX (Pro Tools) is built and supported. Windows is ready as of v2.03-beta.

---

## 3. Color parameter description (optional refinement)

**Current:**
```
Color | 0 – 100% (per-engine: saturation, tape tone, warp, etc.)
```

**Optional replacement (more precise):**
```
Color | 0 – 100% (per-engine: Green=Bloom, Blue=Focus, Red NQ=saturation, Red HQ=tape tone, Purple=warp, Black=mod intensity)
```

**Reason:** Matches the implemented color semantics. The current text is acceptable but less specific.

---

## Verified as correct (no change needed)

| Item | Value |
|------|-------|
| Rate | 0.01 – 20 Hz ✓ |
| Sample rate | 44.1 kHz – 192 kHz ✓ |
| Bit depth | 32-bit float ✓ |
| Channels | Stereo (L/R quadrature LFO) ✓ |
| Block size | Up to 4096 samples ✓ |
| Reported latency | 0 samples ✓ |
| Depth | 0 – 100% ✓ |
| Offset | 0 – 180° ✓ |
| Width | 0 – 200% ✓ |
| Mix | 0 – 100% ✓ |
| Algorithms table | Green/Blue/Red/Purple/Black NQ/HQ ✓ |
| Dev Panel | value FX presets ✓ |
| Type values, Engine persistence, Feedback, Help/About, Animated UI | ✓ |
