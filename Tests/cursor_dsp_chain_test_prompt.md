# Cursor: Build and Run DSP Chain Topology Test

## Context
A new CMake target `ChoroborosDspChainTest` has been added to `CMakeLists.txt`. It lives at `Tests/DspChainTest.cpp` and links against the full Choroboros shared code target. It needs JUCE and all the same dependencies as the main plugin.

## Step 1: Build

```bash
cd /path/to/choroboros-open-source
cmake --build build --target ChoroborosDspChainTest
```

If the build directory doesn't exist or CMake cache is stale:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target ChoroborosDspChainTest
```

### Troubleshooting build failures

1. **Missing `ApplyContext.h` or `PresetState.h`**: These are in `Source/Plugin/`. The target has `target_include_directories(... PRIVATE Source)` so includes should resolve.
2. **`applyPresetState` not found**: Check that `PluginProcessor.h` declares `bool applyPresetState(const PresetState& state, ApplyContext context)`. If the signature differs, update the call in `DspChainTest.cpp` line ~175.
3. **Linker errors about `ScopedJuceInitialiser_GUI`**: Make sure `juce::juce_audio_utils` is linked (it is in CMakeLists.txt).
4. **Windows stack overflow**: The `MSVC /STACK:8388608` option is already set in CMakeLists.txt.

## Step 2: Run all three test modes

```bash
# Mode 1: Single preset (Psychedelic) — shows topology + measurements
./build/ChoroborosDspChainTest

# Mode 2: With safety limiter — compare output
./build/ChoroborosDspChainTest --limiter

# Mode 3: All presets comparison table
./build/ChoroborosDspChainTest --all
```

## Step 3: Verify results

### Expected behavior for Psychedelic preset WITHOUT limiter:
- TAP 0 (Input): 0.0 dBFS peak (that's the test sine)
- TAP 1 (Full Chain Output): **should show positive dBFS** (clipping) — this confirms the bug Robin reported
- The tool should auto-detect clipping and re-run with limiter

### Expected behavior WITH limiter:
- TAP 2 (After Safety Limiter): should be ≤ −0.2 dBFS
- Verdict: "OUTPUT SAFE"

### Expected behavior for --all:
- Psychedelic, MaxWidth, MaxDepth, MaxEverything: likely CLIP without limiter
- Classic, Modern, Vintage, Linear, MinimalSafe: likely OK
- With limiter: all should be SAFE

## Step 4: Report back

Please paste the full terminal output from all three runs. I need the exact peak values to validate the safety limiter parameters before integrating it into the production DSP chain.
