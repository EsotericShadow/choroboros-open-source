/*
 * Choroboros — DSP Chain Topology Tester
 * Copyright (C) 2026 Kaizen Strategic AI Inc.
 *
 * Isolated test environment that processes audio through the real DSP chain
 * with measurement taps at every stage. Traces peak and RMS levels through:
 *
 *     Input → HPF → PreSat → [DRY HOLD | WET PATH] → DW Mix →
 *     PeakCatch → Trim → LPF → Width(M/S) → [Safety Limiter] → Output
 *
 * Usage:
 *     cmake --build build --target ChoroborosDspChainTest
 *     ./build/ChoroborosDspChainTest [--limiter] [--preset NAME] [--all]
 *
 * The --limiter flag enables the proposed Option B safety limiter after width.
 * The --all flag runs every preset and prints a comparison table.
 */

#include "Plugin/PluginProcessor.h"
#include "Plugin/PresetState.h"
#include "Plugin/ApplyContext.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <array>
#include <vector>

// ═══════════════════════════════════════════════════════════════════
// Tap point — records peak and RMS at a named location in the chain
// ═══════════════════════════════════════════════════════════════════
struct TapPoint
{
    std::string name;
    std::string description;
    float peakL  = 0.0f;
    float peakR  = 0.0f;
    double sumsqL = 0.0;
    double sumsqR = 0.0;
    int sampleCount = 0;

    void measure (const juce::AudioBuffer<float>& buf)
    {
        for (int ch = 0; ch < std::min (buf.getNumChannels(), 2); ++ch)
        {
            const float* data = buf.getReadPointer (ch);
            float chPeak = 0.0f;
            double chSumsq = 0.0;
            for (int i = 0; i < buf.getNumSamples(); ++i)
            {
                const float v = data[i];
                chPeak = std::max (chPeak, std::abs (v));
                chSumsq += static_cast<double>(v) * v;
            }
            if (ch == 0) { peakL = std::max (peakL, chPeak); sumsqL += chSumsq; }
            else         { peakR = std::max (peakR, chPeak); sumsqR += chSumsq; }
        }
        sampleCount += buf.getNumSamples();
    }

    float rmsL() const { return sampleCount > 0 ? static_cast<float>(std::sqrt (sumsqL / sampleCount)) : 0.0f; }
    float rmsR() const { return sampleCount > 0 ? static_cast<float>(std::sqrt (sumsqR / sampleCount)) : 0.0f; }

    static float todB (float lin) { return lin > 1.0e-10f ? 20.0f * std::log10 (lin) : -200.0f; }
};

// ═══════════════════════════════════════════════════════════════════
// Safety limiter — Production-grade lookahead true-peak design
//
// WHY LOOKAHEAD:
// An instant-attack limiter creates discontinuous gain changes that
// generate aliasing (intermodulation sidebands across the spectrum).
// At 4-6 dB GR this is audible as "fuzztone" on transients.
// Professional limiters (FabFilter Pro-L, Sonnox) solve this with
// lookahead: the audio is delayed so the limiter "sees ahead" and
// applies gain reduction BEFORE the peak arrives.  The gain curve
// ramps smoothly over the lookahead window — no discontinuity.
//
// WHY TRUE-PEAK (4× OVERSAMPLED DETECTION):
// Sample-level peaks can miss inter-sample peaks by +1-3 dB after
// D/A reconstruction (EBU R128 / ITU-R BS.1770).  We upsample the
// detection path 4× with cubic interpolation so the limiter catches
// peaks the DAC would create, not just what the samples show.
//
// ARCHITECTURE:
//   1. Scan the current block at 4× oversampled resolution for
//      the true peak level.
//   2. Compute the required gain reduction from a soft-knee curve.
//   3. Apply a smoothed gain ramp to the DELAYED audio (the audio
//      that enters the delay line first, exits after the lookahead).
//   4. The gain ramp spans the lookahead window, so even the
//      steepest transient gets a smooth, alias-free reduction.
//
// SETTINGS:
//   Threshold: −1.0 dBTP (true peak; knee spans −2.0 to 0.0 dBTP)
//   Ratio:     20:1      (near brick-wall)
//   Knee:      2 dB      (smooth onset — avoids audible threshold)
//   Lookahead: 5 ms      (221 samples @ 44.1 kHz — smooth ramp)
//   Release:   50 ms     (smooth recovery, no pumping on chorus)
//   Latency:   5 ms      (reported to DAW via getLatencySamples)
//
// REFERENCES:
//   - EBU R128: −1 dBTP ceiling for broadcast/streaming
//   - AES77-2023: ≤1 dB of peak limiting before encoding
//   - ITU-R BS.1770-5: 4× oversampled true peak measurement
//   - FabFilter Pro-L 2: lookahead + true peak architecture
//   - Airwindows ClipOnly2: minimal-artifact clipping approach
// ═══════════════════════════════════════════════════════════════════
struct SafetyLimiter
{
    // ── Tuneable parameters ──
    static constexpr float thresholdDb  = -1.0f;
    static constexpr float ratio        = 20.0f;
    static constexpr float kneeDb       = 2.0f;
    static constexpr float lookaheadMs  = 5.0f;
    static constexpr float releaseMs    = 50.0f;

    // ── Derived constants ──
    static constexpr float kneeHalf     = kneeDb * 0.5f;
    static constexpr float threshLow    = thresholdDb - kneeHalf;   // −2.0 dBTP (knee foot)
    static constexpr float threshHigh   = thresholdDb + kneeHalf;   //  0.0 dBTP (full ratio)

    // ── State ──
    std::vector<float> delayBuf;       // Circular delay buffer (per channel)
    int delaySamples   = 1;            // Never 0 — prevents % 0 UB if process() called before prepare()
    int writePos       = 0;

    float currentGainDb = 0.0f;        // Smoothed gain reduction (dB, always ≤ 0)
    float targetGainDb  = 0.0f;        // Target GR from latest peak scan
    float rampPerSample = 0.0f;        // dB/sample ramp rate for the lookahead window
    float releaseCoeff  = 0.0f;        // One-pole release coefficient
    int   holdCounter   = 0;           // Hold gain after attack for delaySamples before releasing

    // Last 3 samples of prior block — lets detectTruePeak see Hermite knots across callbacks
    std::array<float, 3> scanTail {{ 0.0f, 0.0f, 0.0f }};
    std::vector<float>   scanScratch; // 3 + maxBlock, filled in prepare (no RT alloc)

    void prepare (double sampleRate, int maxBlockSamples = 512)
    {
        delaySamples = static_cast<int>(std::ceil (lookaheadMs * 0.001 * sampleRate));
        delayBuf.assign (static_cast<size_t>(delaySamples), 0.0f);
        writePos = 0;

        releaseCoeff = std::exp (-1.0f / static_cast<float>(releaseMs * 0.001 * sampleRate));

        currentGainDb = 0.0f;
        targetGainDb  = 0.0f;
        rampPerSample = 0.0f;
        holdCounter   = 0;
        scanTail.fill (0.0f);
        const int mb = std::max (1, maxBlockSamples);
        scanScratch.assign (static_cast<size_t>(3 + mb), 0.0f);
    }

    int getLatencySamples() const { return delaySamples; }

    // ── True-peak detection: cubic interpolation at 4× ──
    // Returns the maximum |sample| across 4× oversampled reconstruction.
    // Uses cubic Hermite interpolation (same family as Catmull-Rom) which
    // closely approximates the sinc-reconstructed waveform for inter-sample
    // peak detection per ITU-R BS.1770.
    static float cubicHermite (float y0, float y1, float y2, float y3, float t)
    {
        float m0 = 0.5f * (y2 - y0);
        float m1 = 0.5f * (y3 - y1);
        float t2 = t * t;
        float t3 = t2 * t;
        return (2.0f * y1 - 2.0f * y2 + m0 + m1) * t3
             + (-3.0f * y1 + 3.0f * y2 - 2.0f * m0 - m1) * t2
             + m0 * t
             + y1;
    }

    static float detectTruePeak (const float* data, int numSamples)
    {
        float peak = 0.0f;

        // Sample-level peak first
        for (int i = 0; i < numSamples; ++i)
            peak = std::max (peak, std::abs (data[i]));

        // Inter-sample peaks via 4× cubic interpolation
        // We need 4 points: [i-1, i, i+1, i+2]
        for (int i = 1; i < numSamples - 2; ++i)
        {
            float y0 = data[i - 1];
            float y1 = data[i];
            float y2 = data[i + 1];
            float y3 = data[i + 2];

            // Check at 3 intermediate points (t = 0.25, 0.5, 0.75)
            // t = 0.0 and t = 1.0 are the sample values already checked above
            for (int f = 1; f <= 3; ++f)
            {
                float t = static_cast<float>(f) * 0.25f;
                float interp = cubicHermite (y0, y1, y2, y3, t);
                peak = std::max (peak, std::abs (interp));
            }
        }

        return peak;
    }

    // ── Soft-knee gain reduction (dB) from a given peak level ──
    static float computeGainReductionDb (float peakDb)
    {
        if (peakDb <= threshLow)
            return 0.0f;

        if (peakDb >= threshHigh)
            return (peakDb - thresholdDb) * (1.0f - 1.0f / ratio);

        // Inside knee: quadratic ramp
        float x = peakDb - threshLow;
        return (1.0f - 1.0f / ratio) * x * x / (2.0f * kneeDb);
    }

    // ── Process one sample with lookahead delay ──
    // Call scanBlock() first to set targetGainDb and rampPerSample
    // for the current block, then call this per-sample.
    //
    // Three-phase envelope: attack ramp → hold → release
    float process (float sample)
    {
        if (delayBuf.empty()) return sample;  // Safety: not yet prepared

        // Write to delay buffer
        delayBuf[static_cast<size_t>(writePos)] = sample;
        writePos = (writePos + 1) % delaySamples;

        // Read from delay buffer (delaySamples behind = the lookahead)
        float delayed = delayBuf[static_cast<size_t>(writePos)];

        // Three-phase gain envelope
        if (targetGainDb < currentGainDb)
        {
            // Phase 1: Attack — ramp down over the lookahead window
            currentGainDb += rampPerSample;
            if (currentGainDb < targetGainDb)
                currentGainDb = targetGainDb;
        }
        else if (holdCounter > 0)
        {
            // Phase 2: Hold — keep gain locked while delayed hot samples pass through
            --holdCounter;
        }
        else
        {
            // Phase 3: Release — exponential decay toward 0 dB
            currentGainDb = releaseCoeff * currentGainDb;
            // Snap to zero when very small to avoid denormals
            if (currentGainDb > -0.001f)
                currentGainDb = 0.0f;
        }

        // Apply gain
        if (currentGainDb < -0.001f)
        {
            float gainLin = std::pow (10.0f, currentGainDb / 20.0f);
            return delayed * gainLin;
        }
        return delayed;
    }

    // ── Block-level: scan for true peak, set ramp target ──
    // Call this ONCE per block BEFORE the per-sample process() loop.
    //
    // Cross-block scan: prepends the last 3 samples from the previous block
    // so that cubic Hermite true-peak detection can see inter-sample peaks
    // that straddle block boundaries.
    //
    // Hold stage: after an attack, hold gain for at least delaySamples
    // before releasing. This ensures the delayed version of the hot
    // signal passes through with full gain reduction applied.
    void scanBlock (const float* data, int numSamples)
    {
        if (numSamples < 1 || scanScratch.size() < static_cast<size_t>(3 + numSamples))
            return;

        // Prepend tail from previous block for cross-boundary true-peak detection
        std::copy (scanTail.begin(), scanTail.end(), scanScratch.begin());
        std::copy (data, data + numSamples, scanScratch.begin() + 3);

        const int padded = 3 + numSamples;
        float truePeak = detectTruePeak (scanScratch.data(), padded);

        // Refresh tail for next block
        scanTail[0] = scanScratch[static_cast<size_t>(padded - 3)];
        scanTail[1] = scanScratch[static_cast<size_t>(padded - 2)];
        scanTail[2] = scanScratch[static_cast<size_t>(padded - 1)];

        if (truePeak < 1.0e-10f)
        {
            // Silent block — allow hold to expire naturally
            rampPerSample = 0.0f;
            return;
        }

        float peakDb = 20.0f * std::log10 (truePeak);
        float neededGrDb = -computeGainReductionDb (peakDb);   // Negative = attenuation

        if (neededGrDb < targetGainDb)
        {
            // Attack: deeper reduction needed → ramp and reset hold
            targetGainDb = neededGrDb;
            rampPerSample = (targetGainDb - currentGainDb)
                          / static_cast<float>(std::max (1, delaySamples));
            holdCounter = delaySamples;  // Hold for one lookahead period after attack
        }
        else
        {
            // No deeper reduction needed — let hold timer expire
            rampPerSample = 0.0f;
        }
    }
};

// ═══════════════════════════════════════════════════════════════════
// Preset definitions (mirrors PresetState.cpp factory presets)
// ═══════════════════════════════════════════════════════════════════
struct PresetParams
{
    const char* name;
    float rate, depth, offset, width, mix, color;
    int   engineColor;   // 0=Green, 1=Blue, 2=Red, 3=Purple, 4=Black
    bool  hq;
};

static const PresetParams kPresets[] =
{
    // Factory presets from PresetState.cpp
    { "Classic",      0.50f, 0.32f, 30.0f, 1.0f, 0.50f, 0.50f, 0, false },
    { "Modern",       0.80f, 0.40f, 45.0f, 1.3f, 0.55f, 0.40f, 1, false },
    { "Vintage",      0.35f, 0.45f, 25.0f, 1.1f, 0.60f, 0.60f, 2, false },
    { "Psychedelic",  0.12f, 0.52f, 52.0f, 2.0f, 0.69f, 0.13f, 3, false },
    { "Linear",       0.60f, 0.30f, 35.0f, 1.0f, 0.45f, 0.00f, 4, false },

    // Stress-test presets
    { "MaxWidth",     0.30f, 0.50f, 90.0f, 2.0f, 0.70f, 0.50f, 3, false },
    { "MaxMix",       0.50f, 0.50f, 45.0f, 1.5f, 1.00f, 0.30f, 0, false },
    { "MaxDepth",     0.10f, 1.00f, 90.0f, 2.0f, 0.80f, 0.50f, 3, false },
    { "MaxEverything", 0.10f, 1.00f, 90.0f, 2.0f, 1.00f, 1.00f, 2, false },
    { "MinimalSafe",  1.00f, 0.10f,  0.0f, 1.0f, 0.20f, 0.00f, 4, false },
};

static constexpr int kNumPresets = sizeof(kPresets) / sizeof(kPresets[0]);

// ═══════════════════════════════════════════════════════════════════
// Full engine × mode matrix
//
// 5 engines × 2 modes = 10 core configurations.  Each uses the same
// "worst-case clipping" parameter set: high mix, max width, deep
// modulation, high stereo offset — the conditions that maximise
// constructive interference and M/S gain.
//
//   Engine  │ NQ Core         │ HQ Core
//   ────────┼─────────────────┼──────────────────
//   Green   │ Lagrange 3rd    │ Lagrange 5th
//   Blue    │ Cubic           │ Thiran Allpass
//   Red     │ BBD             │ Tape
//   Purple  │ Phase Warp      │ Orbit
//   Black   │ Linear          │ Linear Ensemble
// ═══════════════════════════════════════════════════════════════════

// Worst-case param set for clipping: high mix, max width, deep mod, wide stereo.
// This is the "Psychedelic-style" parameter space that triggers the bug.
static constexpr float kStressRate   = 0.12f;
static constexpr float kStressDepth  = 0.70f;
static constexpr float kStressOffset = 90.0f;
static constexpr float kStressWidth  = 2.0f;
static constexpr float kStressMix    = 0.80f;
static constexpr float kStressColor  = 0.50f;

struct MatrixEntry
{
    const char* name;
    int   engineColor;
    bool  hq;
    const char* coreName;
};

static const MatrixEntry kFullMatrix[] =
{
    { "Green NQ",   0, false, "Lagrange 3rd"    },
    { "Green HQ",   0, true,  "Lagrange 5th"    },
    { "Blue NQ",    1, false, "Cubic"           },
    { "Blue HQ",    1, true,  "Thiran Allpass"  },
    { "Red NQ",     2, false, "BBD"             },
    { "Red HQ",     2, true,  "Tape"            },
    { "Purple NQ",  3, false, "Phase Warp"      },
    { "Purple HQ",  3, true,  "Orbit"           },
    { "Black NQ",   4, false, "Linear"          },
    { "Black HQ",   4, true,  "Linear Ensemble" },
};

static constexpr int kMatrixSize = sizeof(kFullMatrix) / sizeof(kFullMatrix[0]);

// ═══════════════════════════════════════════════════════════════════
// Parameter sweep definitions
//
// For each of the 6 user-facing parameters, sweep across 5 values
// while holding all OTHER params at worst-case stress levels.
// This isolates which parameter ranges push each engine into clipping.
// ═══════════════════════════════════════════════════════════════════

struct ParamSweepDef
{
    const char* paramName;
    int   numSteps;
    float values[7];   // up to 7 sweep points

    // Build a PresetParams from a matrix entry + this sweep point
    PresetParams makePreset (const MatrixEntry& engine, int step) const
    {
        PresetParams p;
        p.name        = engine.name;    // overwritten by caller for display
        p.rate        = kStressRate;
        p.depth       = kStressDepth;
        p.offset      = kStressOffset;
        p.width       = kStressWidth;
        p.mix         = kStressMix;
        p.color       = kStressColor;
        p.engineColor = engine.engineColor;
        p.hq          = engine.hq;

        float v = values[step];

        if      (std::string(paramName) == "Rate")   p.rate   = v;
        else if (std::string(paramName) == "Depth")  p.depth  = v;
        else if (std::string(paramName) == "Offset") p.offset = v;
        else if (std::string(paramName) == "Width")  p.width  = v;
        else if (std::string(paramName) == "Mix")    p.mix    = v;
        else if (std::string(paramName) == "Color")  p.color  = v;

        return p;
    }
};

static const ParamSweepDef kParamSweeps[] =
{
    { "Rate",   5, { 0.05f, 0.12f, 0.50f, 1.00f, 2.00f } },
    { "Depth",  5, { 0.10f, 0.30f, 0.52f, 0.70f, 1.00f } },
    { "Offset", 5, { 0.0f, 30.0f, 52.0f, 90.0f, 120.0f } },
    { "Width",  5, { 0.50f, 1.00f, 1.50f, 2.00f, 2.00f } },  // 2.0 is max
    { "Mix",    5, { 0.20f, 0.45f, 0.69f, 0.80f, 1.00f } },
    { "Color",  5, { 0.00f, 0.13f, 0.25f, 0.50f, 1.00f } },
};

static constexpr int kNumParamSweeps = sizeof(kParamSweeps) / sizeof(kParamSweeps[0]);

// ═══════════════════════════════════════════════════════════════════
// Test runner — processes audio through the real ChorusDSP chain
// with instrumentation taps
// ═══════════════════════════════════════════════════════════════════

static constexpr double kSampleRate = 44100.0;
static constexpr int    kBlockSize  = 512;
static constexpr int    kNumBlocks  = 400;  // ~4.6 seconds

// ═══════════════════════════════════════════════════════════════════
// Test signal generators
//
// A pure sine only exercises one comb-filter relationship — useless
// for finding peak-clipping bugs in a chorus.  Real audio has hundreds
// of spectral components hitting constructive interference at different
// delay times simultaneously.  We need broadband signals.
// ═══════════════════════════════════════════════════════════════════
enum class TestSignal
{
    Sine440,       // Single tone — baseline reference, won't clip
    WhiteNoise,    // Broadband — the best stress test for chorus peaks
    PinkNoise,     // Weighted broadband — closer to real music spectrum
    MultiTone,     // 8 harmonically-unrelated tones at equal amplitude
    Impulse,       // Single-sample impulse every 4096 samples — tests transient response
    DenseSweep,    // Linear sweep 20-20k over the full duration — hits every comb null
};

static const char* signalName (TestSignal s)
{
    switch (s)
    {
        case TestSignal::Sine440:    return "440 Hz sine";
        case TestSignal::WhiteNoise: return "White noise (seeded)";
        case TestSignal::PinkNoise:  return "Pink noise (seeded)";
        case TestSignal::MultiTone:  return "8-tone multisine";
        case TestSignal::Impulse:    return "Periodic impulse";
        case TestSignal::DenseSweep: return "Linear sweep 20-20k Hz";
        default: return "Unknown";
    }
}

// Simple xorshift32 PRNG — deterministic, no stdlib dependency.
struct Xorshift32
{
    uint32_t state;
    explicit Xorshift32 (uint32_t seed = 0x12345678u) : state (seed) {}
    uint32_t next()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    // Returns uniform float in [-1, +1]
    float nextFloat() { return static_cast<float>(next()) / 2147483648.0f - 1.0f; }
};

// Paul Kellet's pink noise filter (3 dB/octave rolloff from white noise).
struct PinkFilter
{
    float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
    float process (float white)
    {
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;
        float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        b6 = white * 0.115926f;
        return pink * 0.11f;  // Normalize to roughly ±1
    }
};

static void fillTestSignal (juce::AudioBuffer<float>& buffer, TestSignal signal,
                            int globalSampleOffset, Xorshift32& rng, PinkFilter& pinkL, PinkFilter& pinkR)
{
    const int n = buffer.getNumSamples();
    const double twoPi = 2.0 * juce::MathConstants<double>::pi;

    switch (signal)
    {
        case TestSignal::Sine440:
        {
            for (int i = 0; i < n; ++i)
            {
                double t = (globalSampleOffset + i) / kSampleRate;
                float v = static_cast<float>(std::sin (twoPi * 440.0 * t));
                buffer.setSample (0, i, v);
                buffer.setSample (1, i, v);
            }
            break;
        }

        case TestSignal::WhiteNoise:
        {
            for (int i = 0; i < n; ++i)
            {
                buffer.setSample (0, i, rng.nextFloat());
                buffer.setSample (1, i, rng.nextFloat());
            }
            break;
        }

        case TestSignal::PinkNoise:
        {
            for (int i = 0; i < n; ++i)
            {
                buffer.setSample (0, i, pinkL.process (rng.nextFloat()));
                buffer.setSample (1, i, pinkR.process (rng.nextFloat()));
            }
            break;
        }

        case TestSignal::MultiTone:
        {
            // 8 harmonically-unrelated frequencies spanning the audible range.
            // Chosen to be non-integer-related so they create dense beating patterns.
            static constexpr double freqs[] = { 83.0, 197.0, 440.0, 789.0, 1567.0, 3135.0, 5919.0, 11839.0 };
            static constexpr int numFreqs = 8;
            const float scale = 1.0f / static_cast<float>(numFreqs);  // Normalize so sum peaks at ±1

            for (int i = 0; i < n; ++i)
            {
                double t = (globalSampleOffset + i) / kSampleRate;
                float sum = 0.0f;
                for (int f = 0; f < numFreqs; ++f)
                    sum += static_cast<float>(std::sin (twoPi * freqs[f] * t));
                sum *= scale;
                buffer.setSample (0, i, sum);
                buffer.setSample (1, i, sum);
            }
            break;
        }

        case TestSignal::Impulse:
        {
            buffer.clear();
            for (int i = 0; i < n; ++i)
            {
                int globalIdx = globalSampleOffset + i;
                if (globalIdx % 4096 == 0)
                {
                    buffer.setSample (0, i, 1.0f);
                    buffer.setSample (1, i, 1.0f);
                }
            }
            break;
        }

        case TestSignal::DenseSweep:
        {
            // Linear sweep from 20 Hz to 20 kHz over the full test duration.
            const double totalSamples = kNumBlocks * kBlockSize;
            for (int i = 0; i < n; ++i)
            {
                double pos = (globalSampleOffset + i) / totalSamples;  // 0..1
                double freq = 20.0 + pos * (20000.0 - 20.0);
                double t = (globalSampleOffset + i) / kSampleRate;
                float v = static_cast<float>(std::sin (twoPi * freq * t));
                buffer.setSample (0, i, v);
                buffer.setSample (1, i, v);
            }
            break;
        }
    }
}

struct TestResult
{
    std::string presetName;
    std::string signalName;
    std::vector<TapPoint> taps;
    bool usedLimiter = false;
};

// ═══════════════════════════════════════════════════════════════════
// WAV file loader — reads a .wav file into an AudioBuffer for use
// as a test signal.  Falls back to white noise if the file can't
// be read.
// ═══════════════════════════════════════════════════════════════════
static std::unique_ptr<juce::AudioBuffer<float>> loadWavFile (const std::string& path)
{
    juce::File file (path);
    if (! file.existsAsFile())
    {
        std::cerr << "  WAV file not found: " << path << "\n";
        return nullptr;
    }

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (
        formatManager.createReaderFor (file));

    if (reader == nullptr)
    {
        std::cerr << "  Cannot read audio file: " << path << "\n";
        return nullptr;
    }

    auto buffer = std::make_unique<juce::AudioBuffer<float>> (
        static_cast<int>(reader->numChannels),
        static_cast<int>(reader->lengthInSamples));

    reader->read (buffer.get(), 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

    std::cout << "  Loaded: " << file.getFileName() << " ("
              << reader->numChannels << "ch, "
              << reader->sampleRate << " Hz, "
              << reader->lengthInSamples << " samples, "
              << std::fixed << std::setprecision(1)
              << (reader->lengthInSamples / reader->sampleRate) << " sec)\n";

    // Normalize to 0 dBFS peak
    float peak = 0.0f;
    for (int ch = 0; ch < buffer->getNumChannels(); ++ch)
        peak = std::max (peak, buffer->getMagnitude (ch, 0, buffer->getNumSamples()));

    if (peak > 0.0f && peak != 1.0f)
    {
        float gain = 1.0f / peak;
        for (int ch = 0; ch < buffer->getNumChannels(); ++ch)
            buffer->applyGain (ch, 0, buffer->getNumSamples(), gain);
        std::cout << "  Normalized: peak " << std::setprecision(3) << peak
                  << " → 1.0 (applied " << std::setprecision(1)
                  << (20.0f * std::log10(gain)) << " dB)\n";
    }

    return buffer;
}

// Forward-declare the per-stage measurement approach.
// We hook into the real processor, set params, render blocks, and
// capture intermediate state by re-processing isolated stages.
TestResult runChainTest (const PresetParams& preset, bool useLimiter,
                         TestSignal signal = TestSignal::WhiteNoise,
                         const juce::AudioBuffer<float>* wavBuffer = nullptr)
{
    TestResult result;
    result.presetName = preset.name;
    result.signalName = (wavBuffer != nullptr) ? "WAV file" : signalName (signal);
    result.usedLimiter = useLimiter;

    // ── Create processor and prepare ──
    auto proc = std::make_unique<ChoroborosAudioProcessor>();
    proc->setPlayConfigDetails (2, 2, kSampleRate, kBlockSize);
    proc->prepareToPlay (kSampleRate, kBlockSize);

    // ── Set parameters via PresetState + applyPresetState ──
    PresetState state;
    state.rate             = preset.rate;
    state.depth            = preset.depth;
    state.offset           = preset.offset;
    state.width            = preset.width;
    state.mix              = preset.mix;
    state.color            = preset.color;
    state.engineColorIndex = preset.engineColor;
    state.hqEnabled        = preset.hq;

    proc->applyPresetState (state, ApplyContext::FactoryPresetLoad);

    // Let the processor settle for a few blocks (smoothing, filters prime)
    {
        juce::AudioBuffer<float> warmup (2, kBlockSize);
        juce::MidiBuffer midi;
        for (int i = 0; i < 10; ++i)
        {
            warmup.clear();
            proc->processBlock (warmup, midi);
        }
    }

    // ── Set up taps ──
    result.taps.push_back ({
        "Input",
        std::string ("Test signal: ") + result.signalName + " at 0 dBFS, stereo"
    });

    result.taps.push_back ({
        "Full Chain Output",
        "After: HPF > PreSat > Chorus > WetChar > PostSat > DW Mix > PeakCatch > Trim > LPF > Width"
    });

    if (useLimiter)
    {
        result.taps.push_back ({
            "After Safety Limiter",
            "Lookahead: -1.0 dBTP, 20:1, 2 dB knee, 5 ms lookahead, 4x true-peak, 50 ms release"
        });
    }

    // ── Prepare safety limiter (one per channel) ──
    SafetyLimiter limiterL, limiterR;
    limiterL.prepare (kSampleRate);
    limiterR.prepare (kSampleRate);

    // ── PRNG and pink filter state ──
    Xorshift32 rng (0xDEADBEEFu);
    PinkFilter pinkL, pinkR;

    // ── WAV playback state ──
    int wavReadPos = 0;

    // ── Process blocks ──
    const int numBlocks = (wavBuffer != nullptr)
        ? static_cast<int>(std::ceil (static_cast<double>(wavBuffer->getNumSamples()) / kBlockSize))
        : kNumBlocks;

    for (int block = 0; block < numBlocks; ++block)
    {
        juce::AudioBuffer<float> buffer (2, kBlockSize);

        if (wavBuffer != nullptr)
        {
            // Copy from WAV, zero-pad if at end
            buffer.clear();
            int remaining = wavBuffer->getNumSamples() - wavReadPos;
            int toCopy = std::min (kBlockSize, remaining);
            if (toCopy > 0)
            {
                for (int ch = 0; ch < std::min (buffer.getNumChannels(), wavBuffer->getNumChannels()); ++ch)
                    buffer.copyFrom (ch, 0, *wavBuffer, ch, wavReadPos, toCopy);
                // If WAV is mono, duplicate to R
                if (wavBuffer->getNumChannels() == 1)
                    buffer.copyFrom (1, 0, buffer, 0, 0, toCopy);
            }
            wavReadPos += kBlockSize;
        }
        else
        {
            fillTestSignal (buffer, signal, block * kBlockSize, rng, pinkL, pinkR);
        }

        // TAP 0: Measure input
        result.taps[0].measure (buffer);

        // Process through the real chain
        juce::MidiBuffer midi;
        proc->processBlock (buffer, midi);

        // TAP 1: Measure full output (before limiter)
        result.taps[1].measure (buffer);

        // TAP 2: Apply lookahead safety limiter and measure
        if (useLimiter)
        {
            // Step 1: Scan both channels for the block's true peak,
            //         set the gain ramp target for each channel
            limiterL.scanBlock (buffer.getReadPointer (0), kBlockSize);
            limiterR.scanBlock (buffer.getReadPointer (1), kBlockSize);

            // Step 2: Per-sample processing through the delay + gain ramp
            for (int i = 0; i < kBlockSize; ++i)
            {
                buffer.setSample (0, i, limiterL.process (buffer.getSample (0, i)));
                buffer.setSample (1, i, limiterR.process (buffer.getSample (1, i)));
            }
            result.taps[2].measure (buffer);
        }
    }

    proc->releaseResources();
    return result;
}

// ═══════════════════════════════════════════════════════════════════
// Display helpers
// ═══════════════════════════════════════════════════════════════════

// Format linear amplitude (sample value) as dBFS string.
static std::string dBstr (float linear)
{
    if (linear < 1.0e-10f) return "  -inf  ";
    float db = 20.0f * std::log10 (linear);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << std::setw(7) << db;
    return oss.str();
}

static std::string linstr (float v)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << std::setw(7) << v;
    return oss.str();
}

static void printBar (float dB, float maxdB = 18.0f, int width = 40)
{
    float norm = (dB + 48.0f) / (maxdB + 48.0f);
    int filled = std::max (0, std::min (width, static_cast<int>(norm * width)));
    int zeroPos = static_cast<int>((0.0f + 48.0f) / (maxdB + 48.0f) * width);

    for (int i = 0; i < width; ++i)
    {
        if (i == zeroPos)       std::cout << "|";
        else if (i < filled)    std::cout << (dB > 0.0f && i >= zeroPos ? "#" : "=");
        else                    std::cout << " ";
    }
}

static void printSingleResult (const TestResult& r)
{
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  PRESET: " << std::left << std::setw(54) << r.presetName << "║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";

    for (size_t i = 0; i < r.taps.size(); ++i)
    {
        const auto& tap = r.taps[i];
        bool clips = TapPoint::todB (tap.peakL) > -0.5f || TapPoint::todB (tap.peakR) > -0.5f;
        const char* status = clips ? "!! CLIP" : "   OK  ";

        std::cout << "║                                                                  ║\n";
        std::cout << "║  " << std::left << std::setw(62) << ("TAP " + std::to_string(i) + ": " + tap.name) << "  ║\n";
        std::cout << "║  " << std::left << std::setw(64) << tap.description << "║\n";
        std::cout << "║                                                                  ║\n";
        std::cout << "║  L peak: " << linstr(tap.peakL) << " (" << dBstr(tap.peakL) << " dBFS)  ";
        printBar (TapPoint::todB(tap.peakL));
        std::cout << "  " << status << "\n";
        std::cout << "║  R peak: " << linstr(tap.peakR) << " (" << dBstr(tap.peakR) << " dBFS)  ";
        printBar (TapPoint::todB(tap.peakR));
        std::cout << "  " << status << "\n";
        std::cout << "║  L rms:  " << linstr(tap.rmsL()) << " (" << dBstr(tap.rmsL()) << " dBFS)\n";
        std::cout << "║  R rms:  " << linstr(tap.rmsR()) << " (" << dBstr(tap.rmsR()) << " dBFS)\n";

        if (i < r.taps.size() - 1)
            std::cout << "║  ────────────────────── ↓ ──────────────────────                ║\n";
    }

    // Verdict
    const auto& output = r.taps.back();
    float maxPeak = std::max (output.peakL, output.peakR);
    float maxdB = TapPoint::todB (maxPeak);

    std::cout << "║                                                                  ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";

    if (maxdB > -0.5f)
    {
        std::cout << "║  VERDICT: !! OUTPUT CLIPS !!                                    ║\n";
        std::cout << "║  Peak: " << std::fixed << std::setprecision(1) << maxdB
                  << " dBFS (" << std::setprecision(2) << maxPeak << " linear)"
                  << std::string(std::max(0, 36 - 20), ' ') << "║\n";
        if (!r.usedLimiter)
            std::cout << "║  Re-run with --limiter to test the safety limiter fix           ║\n";
    }
    else
    {
        std::cout << "║  VERDICT: OUTPUT SAFE                                           ║\n";
        std::cout << "║  Peak: " << std::fixed << std::setprecision(1) << maxdB
                  << " dBFS — headroom: " << std::setprecision(1) << (-maxdB)
                  << " dB" << std::string(std::max(0, 30 - 15), ' ') << "║\n";
    }

    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";
}

static void printComparisonTable (const std::vector<TestResult>& results, bool withLimiter)
{
    std::cout << "\n";
    std::cout << "┌──────────────────┬───────────────┬───────────────┬─────────────┬────────┐\n";
    std::cout << "│ Preset           │ Peak L (dBFS) │ Peak R (dBFS) │ Max (dBFS)  │ Status │\n";
    std::cout << "├──────────────────┼───────────────┼───────────────┼─────────────┼────────┤\n";

    for (const auto& r : results)
    {
        const auto& out = r.taps.back();
        float peakLdB = TapPoint::todB (out.peakL);
        float peakRdB = TapPoint::todB (out.peakR);
        float maxdB = std::max (peakLdB, peakRdB);
        const char* status = maxdB > -0.5f ? " CLIP! " : "  OK   ";

        std::cout << "│ " << std::left << std::setw(17) << r.presetName
                  << "│ " << std::right << std::setw(12) << std::fixed << std::setprecision(1) << peakLdB << " "
                  << "│ " << std::setw(12) << peakRdB << " "
                  << "│ " << std::setw(10) << maxdB << "  "
                  << "│" << status << "│\n";
    }

    std::cout << "└──────────────────┴───────────────┴───────────────┴─────────────┴────────┘\n";
    std::cout << "\n";

    if (withLimiter)
        std::cout << "  Safety limiter: ON (-1.0 dBTP, 20:1, 2 dB knee, 5 ms lookahead, 4x true-peak)\n";
    else
        std::cout << "  Safety limiter: OFF — re-run with --limiter to see the fix\n";

    if (!results.empty())
        std::cout << "  Test signal: " << results[0].signalName << ", " << kSampleRate << " Hz, "
                  << kBlockSize << " samples/block\n\n";
}

// ═══════════════════════════════════════════════════════════════════
// Topology reference — printed when running a single preset
// ═══════════════════════════════════════════════════════════════════

static void printTopologyGuide (const PresetParams& p, bool useLimiter)
{
    float dryGain = std::cos (p.mix * juce::MathConstants<float>::halfPi);
    float wetGain = std::sin (p.mix * juce::MathConstants<float>::halfPi);
    float widthComp = 1.0f / std::sqrt (0.5f + 0.5f * p.width * p.width);
    float centreDelayMs = 8.0f + 10.0f * p.depth;
    float satDrive = 1.0f + 3.0f * p.color;

    std::cout << "\n";
    std::cout << "┌─────────────────────────────────────────────────────────────┐\n";
    std::cout << "│              SIGNAL CHAIN TOPOLOGY                          │\n";
    std::cout << "│              " << p.name << " preset @ " << kSampleRate << " Hz"
              << std::string(std::max(0, 33 - static_cast<int>(strlen(p.name)) - 12), ' ')
              << "│\n";
    std::cout << "├─────────────────────────────────────────────────────────────┤\n";
    std::cout << "│                                                             │\n";
    std::cout << "│  ┌─────────┐     Input: 440 Hz sine, 0 dBFS stereo         │\n";
    std::cout << "│  │  INPUT  │                                                │\n";
    std::cout << "│  └────┬────┘                                                │\n";
    std::cout << "│       │                                                     │\n";
    std::cout << "│  ┌────┴────┐     HPF 30 Hz, Q=0.707                        │\n";
    std::cout << "│  │   HPF   │     Removes DC and sub-bass rumble.            │\n";
    std::cout << "│  └────┬────┘                                                │\n";
    std::cout << "│       │                                                     │\n";
    std::cout << "│  ┌────┴────┐     Pre-chorus harmonic warmth.                │\n";
    std::cout << "│  │ PreSat  │     Only active for Red engine (color>0).      │\n";
    std::cout << "│  └────┬────┘                                                │\n";
    std::cout << "│       │                                                     │\n";
    std::cout << "│     ┌─┴─┐        Signal splits here.                        │\n";
    std::cout << "│     │ Y │        Dry copy saved in DryWetMixer.             │\n";
    std::cout << "│     └┬─┬┘        Wet path continues through chorus.         │\n";
    std::cout << "│      │ │                                                    │\n";

    // Dry side
    std::cout << "│  DRY │ │ WET                                                │\n";
    std::cout << "│  ┌───┴┐│         Dry: held at original level.               │\n";
    std::cout << "│  │HOLD││         No processing — passthrough.               │\n";
    std::cout << "│  └───┬┘│                                                    │\n";
    std::cout << "│      │ │                                                    │\n";

    // Wet side
    std::cout << "│      │┌┴────────┐  Pre-emphasis: adaptive HF boost for      │\n";
    std::cout << "│      ││PreEmph  │  quiet signals. Peak 3kHz, Q=0.707.       │\n";
    std::cout << "│      │└┬────────┘                                            │\n";
    std::cout << "│      │ │                                                    │\n";

    const char* coreNames[] = {"Green Lagrange", "Blue Cubic", "Red BBD", "Purple PhaseWarped", "Black Linear"};
    std::cout << "│      │┌┴────────┐  Core: " << coreNames[p.engineColor] << "\n";
    std::cout << "│      ││ CHORUS  │  Centre delay: "
              << std::fixed << std::setprecision(1) << centreDelayMs << " ms\n";
    std::cout << "│      ││  CORE   │  Depth: " << std::setprecision(0) << (p.depth * 100) << "%"
              << "  Offset: " << p.offset << "° (stereo)          │\n";
    std::cout << "│      ││  (LFO)  │  Rate: " << std::setprecision(2) << p.rate << " Hz"
              << "  Catmull-Rom cubic interp.     │\n";
    std::cout << "│      │└┬────────┘                                            │\n";
    std::cout << "│      │ │          << STEREO from here (offset creates L≠R) │\n";
    std::cout << "│      │ │                                                    │\n";
    std::cout << "│      │┌┴────────┐  Wet character shaping. Color: "
              << std::setprecision(0) << (p.color * 100) << "%\n";
    std::cout << "│      ││WetChar  │  Green=Bloom, Blue=Focus, Purple=Warp.    │\n";
    std::cout << "│      │└┬────────┘                                            │\n";
    std::cout << "│      │ │                                                    │\n";
    std::cout << "│      │┌┴────────┐  Post-chorus tanh saturation.             │\n";
    std::cout << "│      ││PostSat  │  Drive: " << std::setprecision(2) << satDrive
              << "x  (Red/Purple NQ only)        │\n";
    std::cout << "│      │└┬────────┘                                            │\n";
    std::cout << "│      │ │                                                    │\n";

    // Merge
    std::cout << "│     ┌┴─┴┐        Dry + Wet merge.                           │\n";
    std::cout << "│     │MIX│        Balanced equal-power (sin/cos crossfade).  │\n";
    std::cout << "│     └─┬─┘        Dry gain: " << std::setprecision(3) << dryGain
              << " (" << std::setprecision(1) << TapPoint::todB(dryGain) << " dB)\n";
    std::cout << "│       │          Wet gain: " << std::setprecision(3) << wetGain
              << " (" << std::setprecision(1) << TapPoint::todB(wetGain) << " dB)\n";
    std::cout << "│       │                                                     │\n";

    std::cout << "│  ┌────┴────┐     Transparent peak shaping compressor.       │\n";
    std::cout << "│  │  PEAK   │     Thresh: -2 dB, Ratio: 2:1, Knee: 4 dB     │\n";
    std::cout << "│  │  CATCH  │     Attack: 1 ms, Release: 100 ms             │\n";
    std::cout << "│  └────┬────┘     Tames chorus peaks but doesn't hard-limit. │\n";
    std::cout << "│       │                                                     │\n";
    std::cout << "│  ┌────┴────┐     User-adjustable output trim.               │\n";
    std::cout << "│  │  TRIM   │     Range: -12 to +12 dB. Default: 0 dB.      │\n";
    std::cout << "│  └────┬────┘                                                │\n";
    std::cout << "│       │                                                     │\n";
    std::cout << "│  ┌────┴────┐     LPF 20 kHz, Q=0.707                       │\n";
    std::cout << "│  │   LPF   │     Anti-aliasing / brightness cap.            │\n";
    std::cout << "│  └────┬────┘                                                │\n";
    std::cout << "│       │                                                     │\n";
    std::cout << "│  ┌────┴────┐     M/S stereo width processing.               │\n";
    std::cout << "│  │  WIDTH  │     mid=(L+R)/2, side=(L-R)/2                  │\n";
    std::cout << "│  │  (M/S)  │     side *= " << std::setprecision(1) << p.width
              << "  comp: " << std::setprecision(3) << widthComp
              << " (" << std::setprecision(1) << TapPoint::todB(widthComp) << " dB)\n";
    std::cout << "│  └────┬────┘     Energy comp preserves RMS, NOT peaks.      │\n";

    if (useLimiter)
    {
        std::cout << "│       │                                                     │\n";
        std::cout << "│  ┌────┴────┐     PROPOSED FIX: lookahead true-peak limiter.  │\n";
        std::cout << "│  │ SAFETY  │     Thresh: -1.0 dBTP, Ratio: 20:1            │\n";
        std::cout << "│  │ LIMITER │     Knee: 2 dB, Lookahead: 5 ms, Rel: 50 ms  │\n";
        std::cout << "│  │ (LA+TP) │     4x true-peak detection (ITU-R BS.1770)    │\n";
        std::cout << "│  └────┬────┘     Latency: " << std::setw(3) << static_cast<int>(std::ceil(5.0 * kSampleRate / 1000.0))
                  << " samples (" << std::setprecision(1) << 5.0f << " ms)                │\n";
    }

    std::cout << "│       │                                                     │\n";
    std::cout << "│  ┌────┴────┐                                                │\n";
    std::cout << "│  │ OUTPUT  │     >> MEASURED BELOW <<                        │\n";
    std::cout << "│  └─────────┘                                                │\n";
    std::cout << "│                                                             │\n";
    std::cout << "└─────────────────────────────────────────────────────────────┘\n";
}

// ═══════════════════════════════════════════════════════════════════
// Matrix and sweep display helpers
// ═══════════════════════════════════════════════════════════════════

static void printMatrixTable (const std::vector<TestResult>& results, bool withLimiter, const char* title)
{
    std::cout << "\n  " << title << "\n\n";
    std::cout << "┌──────────────────┬──────────────────┬───────────────┬───────────────┬─────────────┬────────┐\n";
    std::cout << "│ Engine           │ Core             │ Peak L (dBFS) │ Peak R (dBFS) │ Max (dBFS)  │ Status │\n";
    std::cout << "├──────────────────┼──────────────────┼───────────────┼───────────────┼─────────────┼────────┤\n";

    for (size_t i = 0; i < results.size(); ++i)
    {
        const auto& r = results[i];
        const auto& out = r.taps.back();
        float peakLdB = TapPoint::todB (out.peakL);
        float peakRdB = TapPoint::todB (out.peakR);
        float maxdB   = std::max (peakLdB, peakRdB);
        const char* status = maxdB > -0.5f ? " CLIP! " : "  SAFE ";

        std::cout << "│ " << std::left << std::setw(17) << r.presetName
                  << "│ " << std::setw(17) << (i < static_cast<size_t>(kMatrixSize) ? kFullMatrix[i].coreName : "?")
                  << "│ " << std::right << std::setw(12) << std::fixed << std::setprecision(1) << peakLdB << " "
                  << "│ " << std::setw(12) << peakRdB << " "
                  << "│ " << std::setw(10) << maxdB << "  "
                  << "│" << status << "│\n";
    }

    std::cout << "└──────────────────┴──────────────────┴───────────────┴───────────────┴─────────────┴────────┘\n";
    std::cout << "  Safety limiter: " << (withLimiter ? "ON" : "OFF") << "\n\n";
}

static void printParamSweepTable (const ParamSweepDef& sweep,
                                  const std::vector<std::vector<float>>& peakGrid,
                                  const std::vector<std::string>& engineNames)
{
    // peakGrid[engineIdx][stepIdx] = max peak dBFS
    std::cout << "\n  Parameter: " << sweep.paramName << "\n";
    std::cout << "  (All other params at stress levels: rate=" << kStressRate
              << " depth=" << kStressDepth << " offset=" << kStressOffset
              << " width=" << kStressWidth << " mix=" << kStressMix
              << " color=" << kStressColor << ")\n\n";

    // Header row: param values
    std::cout << "  ┌──────────────────┬";
    for (int s = 0; s < sweep.numSteps; ++s)
    {
        std::cout << "──────────";
        std::cout << (s < sweep.numSteps - 1 ? "┬" : "┐");
    }
    std::cout << "\n";

    std::cout << "  │ Engine           │";
    for (int s = 0; s < sweep.numSteps; ++s)
    {
        std::ostringstream oss;
        if (sweep.values[s] >= 10.0f)
            oss << std::fixed << std::setprecision(0) << sweep.values[s];
        else
            oss << std::fixed << std::setprecision(2) << sweep.values[s];
        std::cout << std::setw(8) << oss.str() << "  │";
    }
    std::cout << "\n";

    std::cout << "  ├──────────────────┼";
    for (int s = 0; s < sweep.numSteps; ++s)
    {
        std::cout << "──────────";
        std::cout << (s < sweep.numSteps - 1 ? "┼" : "┤");
    }
    std::cout << "\n";

    // Data rows
    for (size_t e = 0; e < engineNames.size(); ++e)
    {
        std::cout << "  │ " << std::left << std::setw(17) << engineNames[e] << "│";
        for (int s = 0; s < sweep.numSteps; ++s)
        {
            float dB = peakGrid[e][s];
            bool clips = dB > -0.5f;
            std::cout << (clips ? " " : " ");
            std::cout << std::right << std::setw(6) << std::fixed << std::setprecision(1) << dB;
            std::cout << (clips ? "!" : " ") << " │";
        }
        std::cout << "\n";
    }

    std::cout << "  └──────────────────┴";
    for (int s = 0; s < sweep.numSteps; ++s)
    {
        std::cout << "──────────";
        std::cout << (s < sweep.numSteps - 1 ? "┴" : "┘");
    }
    std::cout << "\n";
    std::cout << "  Values marked with ! exceed -0.5 dBFS (clip)\n\n";
}

// ═══════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════

int main (int argc, char* argv[])
{
    // JUCE needs the message manager for some internals
    juce::ScopedJuceInitialiser_GUI init;

    bool useLimiter = false;
    bool runAll = false;
    bool sweepSignals = false;
    bool fullMatrix = false;
    bool paramSweep = false;
    std::string targetPreset = "Psychedelic";
    std::string wavPath;
    TestSignal signal = TestSignal::WhiteNoise;  // Default: broadband, not sine

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--limiter")             useLimiter = true;
        else if (arg == "--all")            runAll = true;
        else if (arg == "--sweep-signals")  sweepSignals = true;
        else if (arg == "--full-matrix")    fullMatrix = true;
        else if (arg == "--param-sweep")    paramSweep = true;
        else if (arg == "--preset" && i + 1 < argc) targetPreset = argv[++i];
        else if (arg == "--wav"    && i + 1 < argc) wavPath = argv[++i];
        else if (arg == "--signal" && i + 1 < argc)
        {
            std::string s = argv[++i];
            if      (s == "sine")      signal = TestSignal::Sine440;
            else if (s == "white")     signal = TestSignal::WhiteNoise;
            else if (s == "pink")      signal = TestSignal::PinkNoise;
            else if (s == "multitone") signal = TestSignal::MultiTone;
            else if (s == "impulse")   signal = TestSignal::Impulse;
            else if (s == "sweep")     signal = TestSignal::DenseSweep;
            else { std::cerr << "Unknown signal: " << s << "\n"; return 1; }
        }
        else if (arg == "--help" || arg == "-h")
        {
            std::cout << "Usage: ChoroborosDspChainTest [OPTIONS]\n\n"
                      << "  --preset NAME    Run a specific preset (default: Psychedelic)\n"
                      << "  --signal TYPE    Test signal type (default: white)\n"
                      << "                   Types: sine, white, pink, multitone, impulse, sweep\n"
                      << "  --wav FILE       Use a .wav file as test signal (normalized to 0 dBFS)\n"
                      << "  --limiter        Enable the proposed safety limiter after width\n"
                      << "  --all            Run all presets and print comparison table\n"
                      << "  --sweep-signals  Run all 6 signal types for the selected preset\n"
                      << "  --full-matrix    Run all 10 engine x mode combos (stress params)\n"
                      << "  --param-sweep    Sweep each parameter across all 10 engines\n"
                      << "  --help           Show this help\n\n"
                      << "Available presets:\n";
            for (int p = 0; p < kNumPresets; ++p)
                std::cout << "  " << kPresets[p].name << "\n";
            std::cout << "\nExamples:\n"
                      << "  ./ChoroborosDspChainTest                              # Psychedelic + white noise\n"
                      << "  ./ChoroborosDspChainTest --signal sine                 # pure sine baseline\n"
                      << "  ./ChoroborosDspChainTest --wav ~/Music/guitar.wav      # real audio\n"
                      << "  ./ChoroborosDspChainTest --all --signal pink           # all presets, pink noise\n"
                      << "  ./ChoroborosDspChainTest --sweep-signals               # all signals, Psychedelic\n"
                      << "  ./ChoroborosDspChainTest --sweep-signals --limiter     # ...with safety limiter\n"
                      << "  ./ChoroborosDspChainTest --full-matrix                 # all 10 engine x mode combos\n"
                      << "  ./ChoroborosDspChainTest --full-matrix --limiter       # ...with safety limiter\n"
                      << "  ./ChoroborosDspChainTest --param-sweep                 # sweep every param x engine\n"
                      << "  ./ChoroborosDspChainTest --param-sweep --limiter       # ...with safety limiter\n";
            return 0;
        }
    }

    // ── Load WAV if specified ──
    std::unique_ptr<juce::AudioBuffer<float>> wavBuffer;
    if (! wavPath.empty())
        wavBuffer = loadWavFile (wavPath);

    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════\n";
    std::cout << "  Choroboros DSP Chain Topology Tester v2.05\n";
    std::cout << "  " << kSampleRate << " Hz, " << kBlockSize << " samples/block\n";
    std::cout << "  Test signal: " << (wavBuffer ? ("WAV: " + wavPath) : std::string(signalName(signal)))
              << " at 0 dBFS\n";
    std::cout << "  Safety limiter: " << (useLimiter ? "ON (5 ms lookahead, 4x true-peak)" : "OFF") << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════\n";

    if (paramSweep)
    {
        // ═══════════════════════════════════════════════════════════════
        // PARAM SWEEP: For each of the 6 parameters, sweep across values
        // with all 10 engine×mode combos.  Prints a grid per parameter
        // showing peak dBFS at each (engine, param_value) intersection.
        // ═══════════════════════════════════════════════════════════════
        std::cout << "\n  ══════════════════════════════════════════════════════════\n";
        std::cout << "  PARAMETER SENSITIVITY SWEEP — all engines × all params\n";
        std::cout << "  Signal: " << signalName(signal) << " | Limiter: " << (useLimiter ? "ON" : "OFF") << "\n";
        std::cout << "  ══════════════════════════════════════════════════════════\n";

        // Build engine name list
        std::vector<std::string> engineNames;
        for (int e = 0; e < kMatrixSize; ++e)
            engineNames.push_back (kFullMatrix[e].name);

        int totalRuns = kNumParamSweeps * kMatrixSize * kParamSweeps[0].numSteps;
        int runCount = 0;

        for (int p = 0; p < kNumParamSweeps; ++p)
        {
            const auto& sweep = kParamSweeps[p];
            std::vector<std::vector<float>> peakGrid (kMatrixSize);

            for (int e = 0; e < kMatrixSize; ++e)
            {
                peakGrid[e].resize (sweep.numSteps);

                for (int s = 0; s < sweep.numSteps; ++s)
                {
                    ++runCount;
                    std::cout << "  [" << runCount << "/" << totalRuns << "] "
                              << sweep.paramName << "=" << std::fixed << std::setprecision(2)
                              << sweep.values[s] << " on " << kFullMatrix[e].name << "...\r" << std::flush;

                    PresetParams pp = sweep.makePreset (kFullMatrix[e], s);
                    auto result = runChainTest (pp, useLimiter, signal, wavBuffer.get());
                    const auto& out = result.taps.back();
                    float maxdB = std::max (TapPoint::todB (out.peakL), TapPoint::todB (out.peakR));
                    peakGrid[e][s] = maxdB;
                }
            }

            printParamSweepTable (sweep, peakGrid, engineNames);
        }

        // Summary: find the worst clipping across all sweeps
        std::cout << "\n  ══════════════════════════════════════════════════════════\n";
        std::cout << "  SWEEP COMPLETE — " << totalRuns << " configurations tested\n";
        std::cout << "  ══════════════════════════════════════════════════════════\n\n";
    }
    else if (fullMatrix)
    {
        // ═══════════════════════════════════════════════════════════════
        // FULL MATRIX: All 10 engine×mode combos with stress params.
        // Runs without limiter, then re-runs with limiter for comparison.
        // ═══════════════════════════════════════════════════════════════
        std::cout << "\n  Running all 10 engine x mode combinations...\n";
        std::cout << "  Stress params: rate=" << kStressRate << " depth=" << kStressDepth
                  << " offset=" << kStressOffset << " width=" << kStressWidth
                  << " mix=" << kStressMix << " color=" << kStressColor << "\n\n";

        std::vector<TestResult> matrixResults;
        for (int e = 0; e < kMatrixSize; ++e)
        {
            PresetParams pp;
            pp.name        = kFullMatrix[e].name;
            pp.rate        = kStressRate;
            pp.depth       = kStressDepth;
            pp.offset      = kStressOffset;
            pp.width       = kStressWidth;
            pp.mix         = kStressMix;
            pp.color       = kStressColor;
            pp.engineColor = kFullMatrix[e].engineColor;
            pp.hq          = kFullMatrix[e].hq;

            std::cout << "  [" << (e + 1) << "/" << kMatrixSize << "] "
                      << kFullMatrix[e].name << " (" << kFullMatrix[e].coreName << ")...\n";

            matrixResults.push_back (runChainTest (pp, useLimiter, signal, wavBuffer.get()));
        }

        printMatrixTable (matrixResults, useLimiter, "ENGINE x MODE MATRIX — Stress Parameters");

        // Auto-rerun with limiter for side-by-side
        if (! useLimiter)
        {
            std::cout << "  Re-running all engines WITH safety limiter...\n\n";
            std::vector<TestResult> limResults;

            for (int e = 0; e < kMatrixSize; ++e)
            {
                PresetParams pp;
                pp.name        = kFullMatrix[e].name;
                pp.rate        = kStressRate;
                pp.depth       = kStressDepth;
                pp.offset      = kStressOffset;
                pp.width       = kStressWidth;
                pp.mix         = kStressMix;
                pp.color       = kStressColor;
                pp.engineColor = kFullMatrix[e].engineColor;
                pp.hq          = kFullMatrix[e].hq;

                std::cout << "  [" << (e + 1) << "/" << kMatrixSize << "] "
                          << kFullMatrix[e].name << " + limiter...\n";

                limResults.push_back (runChainTest (pp, true, signal, wavBuffer.get()));
            }

            printMatrixTable (limResults, true, "ENGINE x MODE MATRIX — With Safety Limiter");

            // Delta summary
            std::cout << "  ┌──────────────────┬────────────┬────────────┬────────────┐\n";
            std::cout << "  │ Engine           │ Without    │ With       │ GR (dB)    │\n";
            std::cout << "  ├──────────────────┼────────────┼────────────┼────────────┤\n";
            for (int e = 0; e < kMatrixSize; ++e)
            {
                const auto& rawOut = matrixResults[e].taps.back();
                const auto& limOut = limResults[e].taps.back();
                float rawMax = std::max (TapPoint::todB(rawOut.peakL), TapPoint::todB(rawOut.peakR));
                float limMax = std::max (TapPoint::todB(limOut.peakL), TapPoint::todB(limOut.peakR));
                float gr = rawMax - limMax;

                std::cout << "  │ " << std::left << std::setw(17) << kFullMatrix[e].name
                          << "│ " << std::right << std::setw(8) << std::fixed << std::setprecision(1) << rawMax << " "
                          << "│ " << std::setw(8) << limMax << " "
                          << "│ " << std::setw(8) << gr << " "
                          << "│\n";
            }
            std::cout << "  └──────────────────┴────────────┴────────────┴────────────┘\n\n";
        }
    }
    else if (sweepSignals)
    {
        // ── Sweep all signal types for one preset ──
        const PresetParams* found = nullptr;
        for (int i = 0; i < kNumPresets; ++i)
            if (targetPreset == kPresets[i].name) { found = &kPresets[i]; break; }
        if (!found) { std::cerr << "Unknown preset: " << targetPreset << "\n"; return 1; }

        std::cout << "\n  Sweeping all signal types for preset: " << found->name << "\n\n";

        static const TestSignal allSignals[] = {
            TestSignal::Sine440, TestSignal::WhiteNoise, TestSignal::PinkNoise,
            TestSignal::MultiTone, TestSignal::Impulse, TestSignal::DenseSweep
        };

        std::cout << "┌────────────────────┬───────────────┬───────────────┬─────────────┬────────┐\n";
        std::cout << "│ Signal             │ Peak L (dBFS) │ Peak R (dBFS) │ Max (dBFS)  │ Status │\n";
        std::cout << "├────────────────────┼───────────────┼───────────────┼─────────────┼────────┤\n";

        for (auto sig : allSignals)
        {
            std::cout << "  Running: " << signalName(sig) << "...\r" << std::flush;
            auto r = runChainTest (*found, useLimiter, sig, nullptr);
            const auto& out = r.taps.back();
            float peakLdB = TapPoint::todB (out.peakL);
            float peakRdB = TapPoint::todB (out.peakR);
            float maxdB = std::max (peakLdB, peakRdB);
            const char* status = maxdB > -0.5f ? " CLIP! " : "  OK   ";

            std::cout << "│ " << std::left << std::setw(19) << signalName(sig)
                      << "│ " << std::right << std::setw(12) << std::fixed << std::setprecision(1) << peakLdB << " "
                      << "│ " << std::setw(12) << peakRdB << " "
                      << "│ " << std::setw(10) << maxdB << "  "
                      << "│" << status << "│\n";
        }

        std::cout << "└────────────────────┴───────────────┴───────────────┴─────────────┴────────┘\n";
        std::cout << "  Limiter: " << (useLimiter ? "ON" : "OFF") << "\n\n";
    }
    else if (runAll)
    {
        std::vector<TestResult> results;
        for (int i = 0; i < kNumPresets; ++i)
        {
            std::cout << "  Processing: " << kPresets[i].name << "...\n";
            results.push_back (runChainTest (kPresets[i], useLimiter, signal, wavBuffer.get()));
        }
        printComparisonTable (results, useLimiter);

        // Also run with limiter for side-by-side if limiter was off
        if (!useLimiter)
        {
            std::cout << "  Re-running all presets WITH safety limiter for comparison...\n\n";
            std::vector<TestResult> limResults;
            for (int i = 0; i < kNumPresets; ++i)
            {
                std::cout << "  Processing: " << kPresets[i].name << " (+ limiter)...\n";
                limResults.push_back (runChainTest (kPresets[i], true, signal, wavBuffer.get()));
            }
            printComparisonTable (limResults, true);
        }
    }
    else
    {
        // ── Single preset ──
        const PresetParams* found = nullptr;
        for (int i = 0; i < kNumPresets; ++i)
        {
            if (targetPreset == kPresets[i].name)
            {
                found = &kPresets[i];
                break;
            }
        }
        if (!found)
        {
            std::cerr << "Unknown preset: " << targetPreset << "\n";
            std::cerr << "Available: ";
            for (int i = 0; i < kNumPresets; ++i)
                std::cerr << kPresets[i].name << (i < kNumPresets - 1 ? ", " : "\n");
            return 1;
        }

        printTopologyGuide (*found, useLimiter);
        auto result = runChainTest (*found, useLimiter, signal, wavBuffer.get());
        printSingleResult (result);

        // If clipping and no limiter, automatically show the fix
        const auto& out = result.taps.back();
        float maxPeak = std::max (out.peakL, out.peakR);
        if (TapPoint::todB (maxPeak) > -0.5f && !useLimiter)
        {
            std::cout << "\n  >> Auto-running with safety limiter to show the fix...\n";
            auto fixResult = runChainTest (*found, true, signal, wavBuffer.get());

            const auto& fixOut = fixResult.taps.back();
            float fixMax = std::max (fixOut.peakL, fixOut.peakR);
            std::cout << "\n  With safety limiter:\n";
            std::cout << "    L peak: " << std::fixed << std::setprecision(1)
                      << TapPoint::todB(fixOut.peakL) << " dBFS\n";
            std::cout << "    R peak: " << TapPoint::todB(fixOut.peakR) << " dBFS\n";
            std::cout << "    Gain reduction: "
                      << std::setprecision(1) << (TapPoint::todB(maxPeak) - TapPoint::todB(fixMax))
                      << " dB\n";
            std::cout << "    Status: " << (TapPoint::todB(fixMax) <= -0.5f ? "SAFE" : "STILL CLIPS") << "\n\n";
        }
    }

    return 0;
}
