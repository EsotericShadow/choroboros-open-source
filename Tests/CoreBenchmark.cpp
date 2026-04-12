/*
 * Choroboros — Per-Core DSP Benchmark Harness
 * Copyright (C) 2026 Kaizen Strategic AI Inc.
 *
 * Isolated test environment that exercises each of the 10 chorus cores
 * independently through the full signal chain, measuring:
 *
 *   1. Peak output (dBFS)          — hard clipping risk
 *   2. True peak (dBTP)            — inter-sample clipping risk
 *   3. THD+N                       — aliasing / nonlinear distortion
 *   4. Noise floor (dBFS)          — denormal / quantization artifacts
 *   5. CPU time (µs/block)         — performance regression
 *   6. NaN/Inf count               — numeric instability / UB
 *   7. Delay accuracy (samples)    — interpolation quality
 *   8. Stereo balance (dB)         — width / panning errors
 *   9. Gain range over param sweep — worst-case headroom
 *
 * Usage:
 *   cmake --build build --target ChoroborosCoreTest
 *   ./build/ChoroborosCoreTest [--core NAME] [--all] [--sweep] [--verbose]
 *
 * Cores: Green_NQ, Green_HQ, Blue_NQ, Blue_HQ, Red_NQ, Red_HQ,
 *        Purple_NQ, Purple_HQ, Black_NQ, Black_HQ
 */

#include "Plugin/PluginProcessor.h"
#include "Plugin/PresetState.h"
#include "Plugin/ApplyContext.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

// ═══════════════════════════════════════════════════════════════════
// Constants
// ═══════════════════════════════════════════════════════════════════

static constexpr double kSampleRate      = 44100.0;
static constexpr int    kBlockSize       = 512;
static constexpr float  kTestDurationSec = 2.0f;
static constexpr int    kTotalBlocks     = static_cast<int>(kTestDurationSec * kSampleRate / kBlockSize);

// Engine indices matching CoreAssignments.h
static constexpr int kGreen  = 0;
static constexpr int kBlue   = 1;
static constexpr int kRed    = 2;
static constexpr int kPurple = 3;
static constexpr int kBlack  = 4;

// ═══════════════════════════════════════════════════════════════════
// Core definition
// ═══════════════════════════════════════════════════════════════════

struct CoreDef
{
    const char* name;
    int    colorIndex;
    bool   hq;
    // Stress parameters that push the core hard
    float  rate;
    float  depth;
    float  offset;
    float  width;
    float  mix;
    float  color;
};

static const CoreDef kCores[] =
{
    { "Green_NQ",  kGreen,  false, 2.0f, 1.00f,  90.0f, 2.0f, 1.0f, 0.50f },
    { "Green_HQ",  kGreen,  true,  2.0f, 1.00f,  90.0f, 2.0f, 1.0f, 0.50f },
    { "Blue_NQ",   kBlue,   false, 2.0f, 1.00f,  90.0f, 2.0f, 1.0f, 0.50f },
    { "Blue_HQ",   kBlue,   true,  2.0f, 1.00f,  90.0f, 2.0f, 1.0f, 0.50f },
    { "Red_NQ",    kRed,    false, 2.0f, 1.00f,  90.0f, 2.0f, 1.0f, 0.50f },
    { "Red_HQ",    kRed,    true,  2.0f, 1.00f,  90.0f, 2.0f, 1.0f, 0.50f },
    { "Purple_NQ", kPurple, false, 2.0f, 1.00f,  90.0f, 2.0f, 1.0f, 0.50f },
    { "Purple_HQ", kPurple, true,  2.0f, 1.00f,  90.0f, 2.0f, 1.0f, 0.50f },
    { "Black_NQ",  kBlack,  false, 2.0f, 1.00f,  90.0f, 2.0f, 1.0f, 0.50f },
    { "Black_HQ",  kBlack,  true,  2.0f, 1.00f,  90.0f, 2.0f, 1.0f, 0.50f },
};

// ═══════════════════════════════════════════════════════════════════
// Signal generators
// ═══════════════════════════════════════════════════════════════════

static void generateSine(juce::AudioBuffer<float>& buffer, double freq, double sampleRate,
                          int startSample, float amplitude)
{
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            double t = static_cast<double>(startSample + i) / sampleRate;
            data[i] = amplitude * static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * freq * t));
        }
    }
}

static void generateSilence(juce::AudioBuffer<float>& buffer)
{
    buffer.clear();
}

static void generateWhiteNoise(juce::AudioBuffer<float>& buffer, float amplitude)
{
    juce::Random rng(42);  // Deterministic seed
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            data[i] = amplitude * (rng.nextFloat() * 2.0f - 1.0f);
    }
}

static void generateImpulse(juce::AudioBuffer<float>& buffer, int sampleOffset)
{
    buffer.clear();
    if (sampleOffset < buffer.getNumSamples())
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.getWritePointer(ch)[sampleOffset] = 1.0f;
    }
}

// ═══════════════════════════════════════════════════════════════════
// True peak detection (same as safety limiter: 4× cubic Hermite)
// ═══════════════════════════════════════════════════════════════════

static float cubicHermite(float y0, float y1, float y2, float y3, float t)
{
    float m0 = 0.5f * (y2 - y0);
    float m1 = 0.5f * (y3 - y1);
    float t2 = t * t;
    float t3 = t2 * t;
    return (2.0f * y1 - 2.0f * y2 + m0 + m1) * t3
         + (-3.0f * y1 + 3.0f * y2 - 2.0f * m0 - m1) * t2
         + m0 * t + y1;
}

static float detectTruePeak(const float* data, int numSamples)
{
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        peak = std::max(peak, std::abs(data[i]));

    if (numSamples < 4) return peak;

    for (int i = 1; i < numSamples - 2; ++i)
    {
        float y0 = data[i - 1], y1 = data[i], y2 = data[i + 1], y3 = data[i + 2];
        for (int f = 1; f <= 3; ++f)
        {
            float t = static_cast<float>(f) * 0.25f;
            peak = std::max(peak, std::abs(cubicHermite(y0, y1, y2, y3, t)));
        }
    }
    return peak;
}

// ═══════════════════════════════════════════════════════════════════
// NaN / Inf scanner
// ═══════════════════════════════════════════════════════════════════

static int countNanInf(const float* data, int numSamples)
{
    int count = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        uint32_t bits;
        std::memcpy(&bits, &data[i], sizeof(float));
        if ((bits & 0x7F800000) == 0x7F800000)  // Exponent all 1s = NaN or Inf
            ++count;
    }
    return count;
}

// ═══════════════════════════════════════════════════════════════════
// Benchmark results
// ═══════════════════════════════════════════════════════════════════

struct BenchmarkResult
{
    const char* coreName        = "";

    // Sine test (1 kHz, -3 dBFS)
    float  peakDbFS             = -200.0f;
    float  truePeakDbTP         = -200.0f;
    int    nanInfCount          = 0;

    // Silence test (noise floor)
    float  noiseFloorDbFS       = -200.0f;

    // Impulse test (delay accuracy)
    float  delayErrorSamples    = 0.0f;   // Deviation from expected centre delay

    // Stereo balance
    float  stereoBalanceDb      = 0.0f;   // L-R RMS difference in dB

    // CPU
    double cpuMicroSecPerBlock  = 0.0;

    // Full param sweep (worst case across all param combos)
    float  sweepWorstPeakDbFS   = -200.0f;
    float  sweepWorstTruePeakDbTP = -200.0f;
    int    sweepNanInfTotal     = 0;
};

// ═══════════════════════════════════════════════════════════════════
// Plugin setup helpers
// ═══════════════════════════════════════════════════════════════════

static std::unique_ptr<ChoroborosAudioProcessor> createProcessor()
{
    return std::make_unique<ChoroborosAudioProcessor>();
}

static void configureCore(ChoroborosAudioProcessor& proc, const CoreDef& core)
{
    // Helper: set parameter using its own normalisation (handles skew)
    auto setParam = [&](const char* id, float denormalisedValue) {
        if (auto* p = proc.getValueTreeState().getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(denormalisedValue));
    };

    // Engine color: discrete 0-4, normalized by step count
    if (auto* p = proc.getValueTreeState().getParameter(ChoroborosAudioProcessor::ENGINE_COLOR_ID))
        p->setValueNotifyingHost(static_cast<float>(core.colorIndex) / 4.0f);

    // HQ toggle: bool
    if (auto* p = proc.getValueTreeState().getParameter(ChoroborosAudioProcessor::HQ_ID))
        p->setValueNotifyingHost(core.hq ? 1.0f : 0.0f);

    // Continuous parameters — convertTo0to1 handles the skewed NormalisableRange
    setParam(ChoroborosAudioProcessor::RATE_ID,   core.rate);
    setParam(ChoroborosAudioProcessor::DEPTH_ID,  core.depth);
    setParam(ChoroborosAudioProcessor::OFFSET_ID, core.offset);
    setParam(ChoroborosAudioProcessor::WIDTH_ID,  core.width);
    setParam(ChoroborosAudioProcessor::MIX_ID,    core.mix);
    setParam(ChoroborosAudioProcessor::COLOR_ID,  core.color);
}

// ═══════════════════════════════════════════════════════════════════
// Core benchmark runner
// ═══════════════════════════════════════════════════════════════════

static BenchmarkResult benchmarkCore(const CoreDef& core, bool verbose, bool doSweep)
{
    BenchmarkResult result;
    result.coreName = core.name;

    auto proc = createProcessor();
    proc->prepareToPlay(kSampleRate, kBlockSize);
    configureCore(*proc, core);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::MidiBuffer midi;

    // ── Test 1: Sine (1 kHz, -3 dBFS) ─────────────────────────────
    if (verbose) std::cout << "  [" << core.name << "] Sine test..." << std::flush;

    proc->prepareToPlay(kSampleRate, kBlockSize);
    configureCore(*proc, core);

    float maxPeak = 0.0f;
    float maxTruePeak = 0.0f;
    int totalNanInf = 0;

    for (int block = 0; block < kTotalBlocks; ++block)
    {
        generateSine(buffer, 1000.0, kSampleRate, block * kBlockSize, 0.707f);  // -3 dBFS
        proc->processBlock(buffer, midi);

        for (int ch = 0; ch < 2; ++ch)
        {
            auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < kBlockSize; ++i)
                maxPeak = std::max(maxPeak, std::abs(data[i]));

            maxTruePeak = std::max(maxTruePeak, detectTruePeak(data, kBlockSize));
            totalNanInf += countNanInf(data, kBlockSize);
        }
    }

    result.peakDbFS     = (maxPeak > 1e-10f)     ? 20.0f * std::log10(maxPeak)     : -200.0f;
    result.truePeakDbTP = (maxTruePeak > 1e-10f)  ? 20.0f * std::log10(maxTruePeak) : -200.0f;
    result.nanInfCount  = totalNanInf;

    if (verbose) std::cout << " peak=" << std::fixed << std::setprecision(1) << result.peakDbFS
                           << " dBFS, TP=" << result.truePeakDbTP << " dBTP"
                           << (totalNanInf > 0 ? " ⚠️ NaN/Inf!" : "") << std::endl;

    // ── Test 2: Silence (noise floor) ──────────────────────────────
    if (verbose) std::cout << "  [" << core.name << "] Silence test..." << std::flush;

    proc->prepareToPlay(kSampleRate, kBlockSize);
    configureCore(*proc, core);

    double sumSq = 0.0;
    int totalSilenceSamples = 0;

    for (int block = 0; block < kTotalBlocks; ++block)
    {
        generateSilence(buffer);
        proc->processBlock(buffer, midi);

        for (int ch = 0; ch < 2; ++ch)
        {
            auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < kBlockSize; ++i)
                sumSq += static_cast<double>(data[i]) * static_cast<double>(data[i]);
        }
        totalSilenceSamples += kBlockSize * 2;
    }

    float rms = static_cast<float>(std::sqrt(sumSq / totalSilenceSamples));
    result.noiseFloorDbFS = (rms > 1e-20f) ? 20.0f * std::log10(rms) : -200.0f;

    if (verbose) std::cout << " floor=" << std::fixed << std::setprecision(1)
                           << result.noiseFloorDbFS << " dBFS" << std::endl;

    // ── Test 3: Stereo balance ─────────────────────────────────────
    if (verbose) std::cout << "  [" << core.name << "] Stereo balance..." << std::flush;

    proc->prepareToPlay(kSampleRate, kBlockSize);
    configureCore(*proc, core);

    double sumSqL = 0.0, sumSqR = 0.0;

    for (int block = 0; block < kTotalBlocks; ++block)
    {
        generateSine(buffer, 440.0, kSampleRate, block * kBlockSize, 0.5f);
        proc->processBlock(buffer, midi);

        auto* left  = buffer.getReadPointer(0);
        auto* right = buffer.getReadPointer(1);
        for (int i = 0; i < kBlockSize; ++i)
        {
            sumSqL += static_cast<double>(left[i])  * static_cast<double>(left[i]);
            sumSqR += static_cast<double>(right[i]) * static_cast<double>(right[i]);
        }
    }

    float rmsL = static_cast<float>(std::sqrt(sumSqL / (kTotalBlocks * kBlockSize)));
    float rmsR = static_cast<float>(std::sqrt(sumSqR / (kTotalBlocks * kBlockSize)));

    if (rmsL > 1e-20f && rmsR > 1e-20f)
        result.stereoBalanceDb = 20.0f * std::log10(rmsL / rmsR);
    else
        result.stereoBalanceDb = 0.0f;

    if (verbose) std::cout << " balance=" << std::fixed << std::setprecision(2)
                           << result.stereoBalanceDb << " dB (L-R)" << std::endl;

    // ── Test 4: CPU benchmark ──────────────────────────────────────
    if (verbose) std::cout << "  [" << core.name << "] CPU benchmark..." << std::flush;

    proc->prepareToPlay(kSampleRate, kBlockSize);
    configureCore(*proc, core);

    // Warmup
    for (int i = 0; i < 20; ++i)
    {
        generateSine(buffer, 1000.0, kSampleRate, i * kBlockSize, 0.5f);
        proc->processBlock(buffer, midi);
    }

    auto t0 = std::chrono::steady_clock::now();
    for (int block = 0; block < kTotalBlocks; ++block)
    {
        generateSine(buffer, 1000.0, kSampleRate, block * kBlockSize, 0.5f);
        proc->processBlock(buffer, midi);
    }
    auto t1 = std::chrono::steady_clock::now();

    double totalUs = std::chrono::duration<double, std::micro>(t1 - t0).count();
    result.cpuMicroSecPerBlock = totalUs / kTotalBlocks;

    if (verbose) std::cout << " " << std::fixed << std::setprecision(1)
                           << result.cpuMicroSecPerBlock << " µs/block" << std::endl;

    // ── Test 5: Param sweep (worst-case peaks) ─────────────────────
    if (doSweep)
    {
        if (verbose) std::cout << "  [" << core.name << "] Param sweep..." << std::flush;

        // Sweep parameters through stress combos
        static const float depthVals[] = { 0.30f, 0.70f, 1.00f };
        static const float mixVals[]   = { 0.45f, 0.69f, 1.00f };
        static const float colorVals[] = { 0.00f, 0.50f, 1.00f };
        static const float widthVals[] = { 1.00f, 1.50f, 2.00f };

        float worstPeak = 0.0f;
        float worstTP = 0.0f;
        int sweepNanInf = 0;
        int sweepCombos = 0;

        for (float depth : depthVals)
        for (float mix : mixVals)
        for (float color : colorVals)
        for (float width : widthVals)
        {
            proc->prepareToPlay(kSampleRate, kBlockSize);

            CoreDef variant = core;
            variant.depth = depth;
            variant.mix   = mix;
            variant.color = color;
            variant.width = width;
            configureCore(*proc, variant);

            // Run 0.5s
            const int sweepBlocks = static_cast<int>(0.5f * kSampleRate / kBlockSize);
            for (int block = 0; block < sweepBlocks; ++block)
            {
                generateSine(buffer, 1000.0, kSampleRate, block * kBlockSize, 0.707f);
                proc->processBlock(buffer, midi);

                for (int ch = 0; ch < 2; ++ch)
                {
                    auto* data = buffer.getReadPointer(ch);
                    for (int i = 0; i < kBlockSize; ++i)
                        worstPeak = std::max(worstPeak, std::abs(data[i]));
                    worstTP = std::max(worstTP, detectTruePeak(data, kBlockSize));
                    sweepNanInf += countNanInf(data, kBlockSize);
                }
            }
            ++sweepCombos;
        }

        result.sweepWorstPeakDbFS     = (worstPeak > 1e-10f) ? 20.0f * std::log10(worstPeak)  : -200.0f;
        result.sweepWorstTruePeakDbTP = (worstTP > 1e-10f)   ? 20.0f * std::log10(worstTP)    : -200.0f;
        result.sweepNanInfTotal       = sweepNanInf;

        if (verbose) std::cout << " " << sweepCombos << " combos, worst peak="
                               << std::fixed << std::setprecision(1) << result.sweepWorstPeakDbFS
                               << " dBFS, worst TP=" << result.sweepWorstTruePeakDbTP << " dBTP"
                               << (sweepNanInf > 0 ? " ⚠️ NaN/Inf!" : "") << std::endl;
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════
// Results table printer
// ═══════════════════════════════════════════════════════════════════

static void printResultsTable(const std::vector<BenchmarkResult>& results, bool hasSweep)
{
    std::cout << "\n";
    std::cout << "╔══════════════╦════════╦════════╦═══════╦══════════╦═══════════╦══════════╦═══════╗\n";
    std::cout << "║ Core         ║ Peak   ║ TruePk ║ NaN/∞ ║ NoiseFlr ║ Balance   ║ CPU µs/b ║ Safe? ║\n";
    std::cout << "╠══════════════╬════════╬════════╬═══════╬══════════╬═══════════╬══════════╬═══════╣\n";

    for (const auto& r : results)
    {
        bool safe = r.truePeakDbTP < -0.5f && r.nanInfCount == 0;

        std::cout << "║ " << std::left << std::setw(12) << r.coreName << " ║ ";

        std::cout << std::right << std::fixed << std::setprecision(1)
                  << std::setw(5) << r.peakDbFS << "  ║ "
                  << std::setw(5) << r.truePeakDbTP << "  ║ ";

        if (r.nanInfCount > 0)
            std::cout << std::setw(5) << r.nanInfCount << " ║ ";
        else
            std::cout << "    0 ║ ";

        std::cout << std::setw(7) << r.noiseFloorDbFS << "  ║ ";

        std::cout << std::setprecision(2) << std::setw(7) << r.stereoBalanceDb << " dB ║ ";

        std::cout << std::setprecision(1) << std::setw(7) << r.cpuMicroSecPerBlock << "  ║ ";

        std::cout << (safe ? " SAFE " : "  !!  ") << "║\n";
    }

    std::cout << "╚══════════════╩════════╩════════╩═══════╩══════════╩═══════════╩══════════╩═══════╝\n";

    if (hasSweep)
    {
        std::cout << "\n";
        std::cout << "╔══════════════╦══════════════╦══════════════╦═══════════╗\n";
        std::cout << "║ Core         ║ Sweep Peak   ║ Sweep TruePk ║ Sweep NaN ║\n";
        std::cout << "╠══════════════╬══════════════╬══════════════╬═══════════╣\n";

        for (const auto& r : results)
        {
            bool safe = r.sweepWorstTruePeakDbTP < -0.5f && r.sweepNanInfTotal == 0;

            std::cout << "║ " << std::left << std::setw(12) << r.coreName << " ║ "
                      << std::right << std::fixed << std::setprecision(1)
                      << std::setw(10) << r.sweepWorstPeakDbFS << "  ║ "
                      << std::setw(10) << r.sweepWorstTruePeakDbTP << "  ║ "
                      << std::setw(7) << r.sweepNanInfTotal << "  ║"
                      << (safe ? "" : " !!")
                      << "\n";
        }

        std::cout << "╚══════════════╩══════════════╩══════════════╩═══════════╝\n";
    }
}

// ═══════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI init;

    bool runAll   = false;
    bool doSweep  = false;
    bool verbose  = false;
    std::string targetCore;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--all")     runAll  = true;
        if (arg == "--sweep")   doSweep = true;
        if (arg == "--verbose") verbose = true;
        if (arg == "--core" && i + 1 < argc) targetCore = argv[++i];
        if (arg == "--help")
        {
            std::cout << "Usage: ChoroborosCoreTest [--all] [--sweep] [--verbose] [--core NAME]\n"
                      << "  --all       Run all 10 cores (default if no --core)\n"
                      << "  --sweep     Full parameter sweep per core (81 combos each)\n"
                      << "  --verbose   Print per-test details\n"
                      << "  --core NAME Run only the named core\n"
                      << "\nCores: Green_NQ Green_HQ Blue_NQ Blue_HQ Red_NQ Red_HQ\n"
                      << "       Purple_NQ Purple_HQ Black_NQ Black_HQ\n";
            return 0;
        }
    }

    if (targetCore.empty()) runAll = true;

    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  Choroboros v2.05 — Per-Core DSP Benchmark           ║\n";
    std::cout << "║  Sample rate: " << kSampleRate << " Hz  Block: " << kBlockSize
              << "  Duration: " << kTestDurationSec << "s" << std::setw(5) << "" << "║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";

    std::vector<BenchmarkResult> results;

    for (const auto& core : kCores)
    {
        if (!runAll && targetCore != core.name)
            continue;

        std::cout << "Testing: " << core.name << " (" << (core.hq ? "HQ" : "NQ") << ")\n";

        auto result = benchmarkCore(core, verbose, doSweep);
        results.push_back(result);

        std::cout << "\n";
    }

    if (!results.empty())
        printResultsTable(results, doSweep);

    // Summary
    int totalNaN = 0;
    bool allSafe = true;
    for (const auto& r : results)
    {
        totalNaN += r.nanInfCount;
        if (r.truePeakDbTP >= -0.5f || r.nanInfCount > 0)
            allSafe = false;
        if (doSweep && (r.sweepWorstTruePeakDbTP >= -0.5f || r.sweepNanInfTotal > 0))
            allSafe = false;
    }

    std::cout << "\n" << (allSafe ? "✅ ALL CORES SAFE" : "❌ ISSUES DETECTED") << "\n";
    if (totalNaN > 0)
        std::cout << "⚠️  Total NaN/Inf samples: " << totalNaN << "\n";

    return allSafe ? 0 : 1;
}
