# Cursor: Rebuild DSP Chain Test and Run With Real Audio

## What changed

`Tests/DspChainTest.cpp` has been updated with:

1. **6 test signal types** instead of just a sine: white noise, pink noise, 8-tone multisine, periodic impulse, linear sweep 20-20kHz, and the original sine. Default is now white noise.
2. **WAV file loading** via `--wav PATH` — reads any wav/aif, normalizes to 0 dBFS, runs through the chain.
3. **Signal sweep mode** via `--sweep-signals` — runs all 6 synthetic signals and prints a comparison table.
4. **A real audio file** at `Tests/BREAK_MY_FINALITY.wav` — stereo, 48 kHz, 16-bit, 3:57, peaks at 0 dBFS.

The previous build only used a 440 Hz sine which peaked at -2.7 dBFS (safe) because a single tone only creates one comb-filter interaction. Real broadband audio creates hundreds of simultaneous constructive interference points in the chorus delay — that's what causes the 4.79x clipping Robin reported.

## Step 1: Rebuild

```bash
cmake --build build --target ChoroborosDspChainTest
```

If there are compile errors, the likely causes are:
- `Xorshift32`, `PinkFilter`, `fillTestSignal`, `loadWavFile` are new — make sure the full updated file is on disk
- `TestResult` now has a `signalName` member — if you see errors about that, the struct definition was updated
- WAV loading uses `juce::AudioFormatManager` which comes from `juce::juce_audio_utils` (already linked)

## Step 2: Run these three tests and paste the FULL output

### Test A: Real audio (the most important one)
```bash
./build/ChoroborosDspChainTest --wav Tests/BREAK_MY_FINALITY.wav
```
This should show whether real broadband audio clips through the Psychedelic preset. If it clips, it will auto-rerun with the safety limiter.

### Test B: Signal sweep
```bash
./build/ChoroborosDspChainTest --sweep-signals
```
This runs all 6 synthetic signal types through Psychedelic and prints a comparison table. White noise and multitone should produce higher peaks than the sine.

### Test C: All presets with white noise
```bash
./build/ChoroborosDspChainTest --all
```
This runs all 10 presets (5 factory + 5 stress) with white noise. The `--all` flag auto-reruns with the limiter for side-by-side comparison.

## What I need back

Paste the full terminal output from all three runs. I need:
- Whether BREAK_MY_FINALITY.wav triggers clipping on the Psychedelic preset
- Which synthetic signals trigger clipping (if any)
- Which presets clip with broadband input
- The safety limiter's effect on each

These numbers determine whether we ship the safety limiter in v2.05 and what its parameters should be.
