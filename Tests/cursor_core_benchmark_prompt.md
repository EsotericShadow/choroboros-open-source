# Cursor: Build and Run Per-Core DSP Benchmark

## What this does

Builds and runs the `ChoroborosCoreTest` harness — an isolated test environment that benchmarks each of the 10 chorus cores (5 engines × 2 modes) independently through the full plugin signal chain.

### Measured vectors per core

1. **Peak output (dBFS)** — hard clipping risk
2. **True peak (dBTP)** — inter-sample clipping via 4× cubic Hermite
3. **NaN/Inf count** — numeric instability / undefined behavior
4. **Noise floor (dBFS)** — denormal artifacts, quantization
5. **Stereo balance (dB)** — L-R RMS imbalance
6. **CPU time (µs/block)** — performance regression
7. **Param sweep worst case** — worst peak across 81 parameter combinations

### Files

- `Tests/CoreBenchmark.cpp` — the test harness
- `CMakeLists.txt` — target `ChoroborosCoreTest` added

## Build

```bash
cd /Users/main/Desktop/CHOROS_MASTER/choroboros-open-source
cmake --build build --target ChoroborosCoreTest
```

If cmake cache is stale (new target not found):
```bash
cmake -B build -S .
cmake --build build --target ChoroborosCoreTest
```

## Test runs

### Run 1: All cores, baseline

```bash
./build/ChoroborosCoreTest --all --verbose
```

Expected: All 10 cores show `SAFE` (true peak < -0.5 dBFS, zero NaN/Inf).

### Run 2: Full param sweep

```bash
./build/ChoroborosCoreTest --all --sweep --verbose
```

This runs 81 parameter combinations (3 depths × 3 mixes × 3 colors × 3 widths) per core = 810 total runs. Takes ~2 minutes. Expected: all sweep worst-case true peaks stay below -0.5 dBFS thanks to the safety limiter.

### Run 3: Single-core deep dive (if any core fails)

```bash
./build/ChoroborosCoreTest --core Purple_NQ --sweep --verbose
```

## What to look for

| Vector | Good | Concerning | Fail |
|---|---|---|---|
| Peak dBFS | < -1.0 | -1.0 to -0.5 | > -0.5 |
| True Peak dBTP | < -0.7 | -0.7 to -0.3 | > -0.3 |
| NaN/Inf | 0 | — | > 0 |
| Noise floor | < -90 | -90 to -60 | > -60 |
| Stereo balance | < ±1.0 dB | ±1.0 to ±3.0 | > ±3.0 |
| CPU µs/block | < 100 | 100-500 | > 500 |

## Pass criteria

Exit code 0 = all safe. Exit code 1 = issues detected.

## If build fails

The test harness uses the same includes and link targets as `ChoroborosDspChainTest`. If that target builds, this one should too. The parameter IDs used in `configureCore()` ("engine", "quality", "rate", "depth", "offset", "width", "mix", "color") must match the APVTS parameter IDs in `PluginProcessor.cpp`. If any don't match, the parameter just won't be set (no crash) — check the APVTS layout and fix the string IDs.
