# DSP Chain Topology Tester

Isolated simulation environment that processes audio through the **real Choroboros DSP chain** with measurement taps. Diagnoses the Psychedelic preset clipping bug and validates the proposed safety limiter fix.

## Build

```bash
# From repo root (assumes build/ directory already configured)
cmake --build build --target ChoroborosDspChainTest
```

If no build directory exists yet:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target ChoroborosDspChainTest
```

## Run

```bash
# Default: Psychedelic preset, shows topology + measurements
./build/ChoroborosDspChainTest

# With the proposed safety limiter (Option B)
./build/ChoroborosDspChainTest --limiter

# All presets — comparison table, auto-runs with/without limiter
./build/ChoroborosDspChainTest --all

# Specific preset
./build/ChoroborosDspChainTest --preset MaxEverything

# Stress test + limiter
./build/ChoroborosDspChainTest --preset MaxEverything --limiter
```

## What It Does

1. Creates a real `ChoroborosAudioProcessor` instance
2. Applies preset parameters via `applyPresetState()` (same code path as the preset browser)
3. Generates a 440 Hz stereo sine at 0 dBFS
4. Processes ~4.6 seconds through the full chain (400 blocks × 512 samples)
5. Measures peak and RMS at tap points:
   - **TAP 0**: Input (before processing)
   - **TAP 1**: Full chain output (after width — this is what the DAW sees)
   - **TAP 2**: After safety limiter (if `--limiter` enabled)
6. Prints an ASCII topology diagram with parameter values for the selected preset
7. If clipping is detected without `--limiter`, automatically re-runs with limiter to show the fix

## Available Presets

| Name | Engine | Rate | Depth | Offset | Width | Mix | Color |
|------|--------|------|-------|--------|-------|-----|-------|
| Classic | Green NQ | 0.50 | 0.32 | 30° | 1.0 | 0.50 | 0.50 |
| Modern | Blue NQ | 0.80 | 0.40 | 45° | 1.3 | 0.55 | 0.40 |
| Vintage | Red NQ | 0.35 | 0.45 | 25° | 1.1 | 0.60 | 0.60 |
| **Psychedelic** | **Purple NQ** | **0.12** | **0.52** | **52°** | **2.0** | **0.69** | **0.13** |
| Linear | Black NQ | 0.60 | 0.30 | 35° | 1.0 | 0.45 | 0.00 |
| MaxWidth | Purple NQ | 0.30 | 0.50 | 90° | 2.0 | 0.70 | 0.50 |
| MaxMix | Green NQ | 0.50 | 0.50 | 45° | 1.5 | 1.00 | 0.30 |
| MaxDepth | Purple NQ | 0.10 | 1.00 | 90° | 2.0 | 0.80 | 0.50 |
| MaxEverything | Red NQ | 0.10 | 1.00 | 90° | 2.0 | 1.00 | 1.00 |
| MinimalSafe | Black NQ | 1.00 | 0.10 | 0° | 1.0 | 0.20 | 0.00 |

## Safety Limiter Proposal (Option B)

The test includes a standalone `SafetyLimiter` struct that can be evaluated independently of the main DSP chain. Parameters:

- **Threshold**: −1.0 dBFS (knee spans −2.0 to 0.0 dBFS so stressed peaks land below a strict −0.5 dBFS reporting line)
- **Ratio**: 20:1 (near brick-wall)
- **Knee**: 2 dB (smooth transition)
- **Attack**: **instant** (envelope = \|sample\| on rising peaks — sample-accurate; smoothed attack is wrong for a safety ceiling)
- **Release**: 50 ms (fast recovery, no pumping)

Once validated, this limiter should be integrated into `ChorusDSP::process()` after the `processWidth()` call.
