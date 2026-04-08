# Choroboros Beta — Validation matrix summary

**Date:** 2026-04-07  
**Repo:** `choroboros-open-source`  
**Build:** `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` then `cmake --build build --config Release --parallel` (clean `build/` before configure).  
**JUCE:** 8.0.12 (from `build/_deps/juce-src/CMakeLists.txt` after FetchContent).  
**pluginval:** `/Applications/pluginval.app/Contents/MacOS/pluginval` (v1.0.4).  
**Product:** Choroboros Beta — VST3 and AU under `build/Choroboros_artefacts/Release/`.

## Executive result

**All matrix steps completed with pluginval exit 0 and auval exit 0.**

- **VST3:** pluginval strictness **3, 5, 7, 10** — all **SUCCESS**.  
- **VST3:** Level **5** with `--random-seed` **12345, 67890, 24601** — all **SUCCESS**.  
- **AU:** pluginval **5** and **10** — **SUCCESS** (see warnings below).  
- **Apple auval:** `auval -v aufx ChBr KzDp` — **`AU VALIDATION SUCCEEDED.`**  
- **Edge / stress:** Level 5 with extreme block sizes and sample rates — **SUCCESS** (see below).

This is a strong signal for **release-candidate testing in real DAWs**; it does not replace host-specific QA.

## Log files (this directory)

| Log file | Description |
|----------|-------------|
| `pluginval_vst3_L3.txt` | VST3, strictness 3 |
| `pluginval_vst3_L5.txt` | VST3, strictness 5 |
| `pluginval_vst3_L7.txt` | VST3, strictness 7 |
| `pluginval_vst3_L10.txt` | VST3, strictness 10 |
| `pluginval_vst3_L5_seed_12345.txt` | VST3 L5, seed 12345 |
| `pluginval_vst3_L5_seed_67890.txt` | VST3 L5, seed 67890 |
| `pluginval_vst3_L5_seed_24601.txt` | VST3 L5, seed 24601 |
| `pluginval_au_L5.txt` | AU, strictness 5 |
| `pluginval_au_L10.txt` | AU, strictness 10 |
| `auval_list_choro_head.txt` | `auval -a \| grep -i choro` (registration check) |
| `auval_full.txt` | Full `auval -v aufx ChBr KzDp` |
| `pluginval_vst3_L5_edge_blocks_sr.txt` | VST3 L5 combined: `--block-sizes 1,8192` and `--sample-rates 96000,192000` (single run) |
| `pluginval_block1.txt` | VST3 L5, `--block-sizes 1` only |
| `pluginval_block8192.txt` | VST3 L5, `--block-sizes 8192` only |
| `pluginval_96k.txt` | VST3 L5, `--sample-rates 96000` only |
| `pluginval_192k.txt` | VST3 L5, `--sample-rates 192000` only |

Each log ends with `Exit: N` where **N** is `${PIPESTATUS[0]}` from the `pluginval | tee` or `auval | tee` pipeline (not `tee`’s exit code).

## Fixes already in tree (reference)

- **HQ state restoration / `AudioParameterBool::setValue` fuzz:** `syncBoolParameterFromRestoredApvtsState` after `replaceState` in `PluginProcessor.cpp` (authoritative `PARAM` value → `setValueNotifyingHost` at 0/1).  
- **Validation script:** `scripts/validate_choroboros_macos.sh` uses `${PIPESTATUS[0]}` and aggregates failure exit.

## Other bool parameters

`grep -rn "AudioParameterBool" Source/` — only **HQ** uses `juce::AudioParameterBool` in plugin code (plus the sync helper). No additional bool params required the same restore hook in this codebase.

## Warnings (non-failing)

- **pluginval AU:** `!!! WARNING: Current program is -1... Is this correct?` appears in AU runs; pluginval still reported **SUCCESS** and exit **0**. Worth monitoring in host preset/program behaviour; not treated as a matrix failure here.

## Failures / open issues

**None** in this matrix run.

## Re-run quick reference

```bash
cd choroboros-open-source
PLUGIN="build/Choroboros_artefacts/Release/VST3/Choroboros Beta.vst3"
AU="build/Choroboros_artefacts/Release/AU/Choroboros Beta.component"
PV=/Applications/pluginval.app/Contents/MacOS/pluginval
mkdir -p validation_logs
set +e; "$PV" --validate "$PLUGIN" --strictness-level 10 --timeout-ms 300000 --verbose 2>&1 | tee validation_logs/pluginval_vst3_L10.txt; echo "Exit: ${PIPESTATUS[0]}" | tee -a validation_logs/pluginval_vst3_L10.txt; set -e
```
