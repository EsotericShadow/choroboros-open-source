# Engine Content for "More" Sections — Corrected

Use this for EngineModal.tsx and the beta site. Aligned with whitepaper and implementation (v2.03-beta).

---

## Green — Classic

**Short tag:** Classic

**Normal quality:** 3rd-order Lagrange interpolation. Smooth, musical chorus with a familiar, classic feel. Low CPU, high musicality.

**High quality:** 5th-order Lagrange interpolation. Richer harmonics, smoother modulation, and more depth. The classic sound, refined.

**Color (Bloom):** The Color slider adds bloom—thickness and gentle vintage softening. No saturation. Low values stay clean and airy; high values add density and damping.

**Whitepaper summary:** Smooth, musical, the "reference" chorus. Lagrange interpolation provides high-quality resampling with minimal artifacts.

---

## Blue — Modern

**Short tag:** Modern

**Normal quality:** Cubic interpolation. Clean, transparent chorus that stays out of the way. Ideal for subtle width and movement.

**High quality:** Thiran allpass interpolation. Phase-accurate, phase-linear chorus. Studio-grade clarity and stereo imaging.

**Color (Focus):** The Color slider adds focus—clarity and presence. No saturation. Low values are softer and wider; high values are tighter, brighter, and more articulate.

**Whitepaper summary:** Clean and transparent. Ideal for subtle widening and contemporary mixes.

---

## Red — Vintage

**Short tag:** Vintage

**Normal quality:** BBD emulation with 5th-order cascade filtering. Warm, analog-like chorus. The Color slider controls saturation only—adds drive to the wet path. Great for guitars and synths.

**High quality:** Tape-style chorus (Hermite interpolation, wow, flutter). Tape tone and drive. The Color slider controls tape tone. Rich, vintage character with depth and movement.

**Whitepaper summary:** Analog character through emulation. For warmth, grit, and "that" sound.

---

## Purple — Experimental

**Short tag:** Experimental *(or "Proprietary & Experimental" if you want to emphasize exclusivity)*

**Normal quality:** Phase-Warped Chorus uses non-uniform phase modulation to create evolving, shifting textures that change over time. A new approach to chorus modulation.

**High quality:** Orbit Chorus uses 2D rotating modulation for wide, moving stereo. The modulation path moves in two dimensions instead of a single LFO, giving a distinct, immersive character.

**Color (Warp):** The Color slider controls warp/orbit shape—how much the phase or orbit character is applied.

**Proprietary note:** Proprietary algorithms developed by Kaizen Strategic AI Inc. Exclusive to Choroboros—protected as trade secrets.

**Whitepaper summary:** Non-standard modulation. For sound design and creative effects.

---

## Black — Linear

**Short tag:** Linear

**Normal quality:** Linear interpolation. Fast, efficient, transparent chorus. Low CPU, predictable behavior, easy to dial in.

**High quality:** Linear Ensemble. Multi-voice chorus for width and depth. Clean, spacious, and very efficient.

**Color (Mod intensity):** The Color slider controls modulation intensity and ensemble spread.

**Whitepaper summary:** Transparent and CPU-efficient. For when you need chorus without character—or maximum performance.

---

## Color Parameter Semantics (per engine)

| Engine | Color control |
|--------|----------------|
| Green | Bloom (thickness, damping; no saturation) |
| Blue | Focus (clarity, presence; no saturation) |
| Red NQ | Saturation only |
| Red HQ | Tape tone + drive |
| Purple | Warp / orbit shape |
| Black | Mod intensity / ensemble spread |

