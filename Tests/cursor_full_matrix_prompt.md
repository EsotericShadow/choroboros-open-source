# Cursor: Rebuild and Run Full Engine Matrix + Parameter Sweep

## What changed

`Tests/DspChainTest.cpp` has two new run modes:

1. **`--full-matrix`** — Runs all 10 engine×mode configurations (Green NQ/HQ, Blue NQ/HQ, Red NQ/HQ, Purple NQ/HQ, Black NQ/HQ) with worst-case stress parameters. Prints a comparison table showing peak dBFS for each core. Auto-reruns with the safety limiter for a side-by-side delta table.

2. **`--param-sweep`** — Sweeps each of the 6 user-facing parameters (Rate, Depth, Offset, Width, Mix, Color) across 5 values while holding all other params at stress levels. Does this for ALL 10 engines. Prints a grid per parameter: rows = engines, columns = param values, cells = peak dBFS. This is 300 test configurations total — expect it to take a few minutes.

The safety limiter uses **instant attack** (zero attack on rising peaks, 50 ms smoothed release) and a **−1.0 dBFS threshold** (2 dB knee: −2.0 to 0.0 dBFS) so stressed outputs clear a **−0.5 dBFS** check after limiting.

## Rebuild

```bash
cmake --build build --target ChoroborosDspChainTest
```

## Run these tests and paste the FULL output

### Test A: Full engine matrix (most important — are ALL engines safe with the limiter?)
```bash
./build/ChoroborosDspChainTest --full-matrix
```
This runs all 10 engine×mode combos WITHOUT limiter, prints the table, then re-runs WITH limiter and prints a delta comparison. Key question: does every engine come in **under −0.5 dBFS** (test harness line) with the limiter?

### Test B: Parameter sensitivity sweep (which params cause the most clipping?)
```bash
./build/ChoroborosDspChainTest --param-sweep
```
This sweeps Rate, Depth, Offset, Width, Mix, and Color across 5 values each, for all 10 engines. 300 total runs. Key questions:
- Which parameter has the biggest impact on peak level?
- At what value does each parameter start causing clips?
- Do all engines behave the same, or do some cores clip more than others?

### Test C: Parameter sweep WITH limiter (does the limiter handle all parameter combos?)
```bash
./build/ChoroborosDspChainTest --param-sweep --limiter
```
Same 300 configurations but with the safety limiter active. Every cell should show ≤ -0.5 dBFS. If any cell still clips with the limiter, that's a problem.

### Test D (optional): Full matrix with real audio
```bash
./build/ChoroborosDspChainTest --full-matrix --wav Tests/BREAK_MY_FINALITY.wav
```
Run all 10 engines with the real audio file to see which cores clip with real music.

## What to report

Paste the full terminal output from all runs. The key numbers I need:

1. **Engine matrix without limiter** — which engines clip, and by how much?
2. **Engine matrix with limiter** — do ALL 10 come in safe? What's the gain reduction per engine?
3. **Parameter sweep grids** — which parameter×engine combos produce the worst peaks?
4. **Parameter sweep with limiter** — any cells still clipping?
5. **WAV matrix (if run)** — which engines clip with real audio?

These results determine:
- Whether the safety limiter is sufficient for ALL cores (not just Purple)
- Whether we need per-engine limiter tuning
- Which parameters need range-limiting or warning indicators in the GUI
